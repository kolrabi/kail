//-----------------------------------------------------------------------
//         Color Quantization Demo
//
// Author: Roman Podobedov
// Email: romka@ut.ee
// Romka Graphics: www.ut.ee/~romka
// 
// Also in this file implemented Wu's Color Quantizer algorithm (v. 2)
// For details see Graphics Gems vol. II, pp. 126-133
//
// Wu's Color Quantizer Algorithm:
// Author: Xiaolin Wu
//			Dept. of Computer Science
//			Univ. of Western Ontario
//			London, Ontario N6A 5B7
//			wu@csd.uwo.ca
//			http://www.csd.uwo.ca/faculty/wu/
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// ImageLib Sources
// by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/src/il_quantizer.c
//
// Description:  Heavily modified by Denton Woods.
//
// 20040223 XIX : Modified so it works better with color requests < 256
// pallete always has memory space for 256 entries
// used so we can quant down to 255 colors then add a transparent color in there.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#define MAXCOLOR	256
#define	RED			2
#define	GREEN		1
#define BLUE		0

typedef struct Box
{
    ILint r0;  // min value, exclusive
    ILint r1;  // max value, inclusive
    ILint g0;  
    ILint g1;  
    ILint b0;  
    ILint b1;
    ILint vol;
} Box;

typedef ILint   Histogram [33*33*33];
typedef ILfloat Histogramf[33*33*32];

#define HIST(h, x,y,z) ((h)[x + 33*(y + 33 * z)])

/* Histogram is in elements 1..HISTSIZE along each axis,
 * element 0 is for base or marginal value
 * NB: these must start out 0!
 */

typedef struct QuantContext {

	Histogramf 	gm2;
	Histogram 	wt, mr, mg, mb;

	ILuint		size; //image size
 	ILuint  	K;    //colour look-up table size
	ILushort	*Qadd;

	ILuint 		WindW, WindH, WindD;
	ILubyte   *buffer;
	ILuint    Width, Height, Depth, Comp;
} QuantContext;

// Build 3-D color histogram of counts, r/g/b, c^2
static ILboolean Hist3d(QuantContext *ctx, ILubyte *Ir, ILubyte *Ig, ILubyte *Ib, ILint *vwt, ILint *vmr, ILint *vmg, ILint *vmb, ILfloat *m2)
{
	ILint	ind, r, g, b;
	ILint	inr, ing, inb;
	ILuint table[256];
	ILuint	i;
		
	for (i = 0; i < 256; i++)
	{
		table[i] = i * i;
	}

	ctx->Qadd = (ILushort*)ialloc(sizeof(ILushort) * ctx->size);
	if (ctx->Qadd == NULL)
	{
		return IL_FALSE;
	}
	
  imemclear(ctx->Qadd, sizeof(ILushort) * ctx->size);
        
	for (i = 0; i < ctx->size; i++)
	{
	    r = Ir[i]; g = Ig[i]; b = Ib[i];
	    inr = (r>>3) + 1; 
	    ing = (g>>3) + 1; 
	    inb = (b>>3) + 1; 
	    ctx->Qadd[i] = ind = (ILushort)((inr<<10)+(inr<<6)+inr+(ing<<5)+ing+inb);
	    //[inr][ing][inb]
	    vwt[ind]++;
	    vmr[ind] += r;
	    vmg[ind] += g;
	    vmb[ind] += b;
		m2[ind] += (ILfloat)(table[r]+table[g]+table[b]);
	}
	return IL_TRUE;
}

/* At conclusion of the histogram step, we can interpret
 *   wt[r][g][b] = sum over voxel of P(c)
 *   mr[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mg, mb
 *   m2[r][g][b] = sum over voxel of c^2*P(c)
 * Actually each of these should be divided by 'size' to give the usual
 * interpretation of P() as ranging from 0 to 1, but we needn't do that here.
 */

/* We now convert histogram into moments so that we can rapidly calculate
 * the sums of the above quantities over any desired Box.
 */


// Compute cumulative moments
static void M3d(ILint *vwt, ILint *vmr, ILint *vmg, ILint *vmb, ILfloat *m2)
{
	ILushort	ind1, ind2;
	ILubyte		i, r, g, b;
	ILint		line, line_r, line_g, line_b, area[33], area_r[33], area_g[33], area_b[33];
	ILfloat		line2, area2[33];

	for (r = 1; r <= 32; r++) {
		for (i = 0; i <= 32; i++) {
			area2[i] = 0.0f;
			area[i]=area_r[i]=area_g[i]=area_b[i]=0;
		}
		for (g = 1; g <= 32; g++) {
			line2 = 0.0f;
			line = line_r = line_g = line_b = 0;
			for (b = 1; b <= 32; b++) {
				ind1 = (ILushort)((r<<10) + (r<<6) + r + (g<<5) + g + b); // [r][g][b]
				line += vwt[ind1];
				line_r += vmr[ind1]; 
				line_g += vmg[ind1]; 
				line_b += vmb[ind1];
				line2 += m2[ind1];
				area[b] += line;
				area_r[b] += line_r;
				area_g[b] += line_g;
				area_b[b] += line_b;
				area2[b] += line2;
				ind2 = ind1 - 1089; // [r-1][g][b]
				vwt[ind1] = vwt[ind2] + area[b];
				vmr[ind1] = vmr[ind2] + area_r[b];
				vmg[ind1] = vmg[ind2] + area_g[b];
				vmb[ind1] = vmb[ind2] + area_b[b];
				m2[ind1] = m2[ind2] + area2[b];
			}
		}
    }

	return;
}


// Compute sum over a Box of any given statistic
static ILint Vol(const Box *cube, const Histogram mmt) 
{
    return(HIST(mmt, cube->r1, cube->g1, cube->b1)
	   			-HIST(mmt, cube->r1, cube->g1, cube->b0)
	   			-HIST(mmt, cube->r1, cube->g0, cube->b1)
	   			+HIST(mmt, cube->r1, cube->g0, cube->b0)
	   			-HIST(mmt, cube->r0, cube->g1, cube->b1)
				  +HIST(mmt, cube->r0, cube->g1, cube->b0)
	   			+HIST(mmt, cube->r0, cube->g0, cube->b1)
	   			-HIST(mmt, cube->r0, cube->g0, cube->b0) );
}

/* The next two routines allow a slightly more efficient calculation
 * of Vol() for a proposed subBox of a given Box.  The sum of Top()
 * and Bottom() is the Vol() of a subBox split in the given direction
 * and with the specified new upper bound.
 */

// Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1
//	(depending on dir)
static ILint Bottom(const Box *cube, ILubyte dir, const Histogram mmt)
{
    switch(dir)
    {
		case RED:
			return( -HIST(mmt, cube->r0, cube->g1, cube->b1)
							+HIST(mmt, cube->r0, cube->g1, cube->b0)
							+HIST(mmt, cube->r0, cube->g0, cube->b1)
							-HIST(mmt, cube->r0, cube->g0, cube->b0) );
		case GREEN:
			return( -HIST(mmt, cube->r1, cube->g0, cube->b1)
							+HIST(mmt, cube->r1, cube->g0, cube->b0)
							+HIST(mmt, cube->r0, cube->g0, cube->b1)
							-HIST(mmt, cube->r0, cube->g0, cube->b0) );
		case BLUE:
			return( -HIST(mmt, cube->r1, cube->g1, cube->b0)
							+HIST(mmt, cube->r1, cube->g0, cube->b0)
							+HIST(mmt, cube->r0, cube->g1, cube->b0)
							-HIST(mmt, cube->r0, cube->g0, cube->b0) );
    }
    return 0;
}


// Compute remainder of Vol(cube, mmt), substituting pos for
//	r1, g1, or b1 (depending on dir)
static ILint Top(const Box *cube, ILubyte dir, ILint pos, const Histogram mmt)
{
    switch (dir)
    {
		case RED:
			return(HIST(mmt, pos, cube->g1, cube->b1) 
			   		-HIST(mmt, pos, cube->g1, cube->b0)
			   		-HIST(mmt, pos, cube->g0, cube->b1)
			   		+HIST(mmt, pos, cube->g0, cube->b0) );
		case GREEN:
			return(HIST(mmt, cube->r1, pos, cube->b1) 
			   		-HIST(mmt, cube->r1, pos, cube->b0)
				    -HIST(mmt, cube->r0, pos, cube->b1)
				    +HIST(mmt, cube->r0, pos, cube->b0) );
		case BLUE:
			return(HIST(mmt, cube->r1, cube->g1, pos)
			   		-HIST(mmt, cube->r1, cube->g0, pos)
			   		-HIST(mmt, cube->r0, cube->g1, pos)
			   		+HIST(mmt, cube->r0, cube->g0, pos) );
    }

    return 0;
}


// Compute the weighted variance of a Box
//	NB: as with the raw statistics, this is really the variance * size
static ILfloat Var(QuantContext *ctx, const Box *cube)
{
	ILfloat dr, dg, db, xx;

	dr = (ILfloat)(Vol(cube, ctx->mr)); 
	dg = (ILfloat)(Vol(cube, ctx->mg)); 
	db = (ILfloat)(Vol(cube, ctx->mb));

	xx = HIST(ctx->gm2, cube->r1, cube->g1, cube->b1) 
		  -HIST(ctx->gm2, cube->r1, cube->g1, cube->b0)
			-HIST(ctx->gm2, cube->r1, cube->g0, cube->b1)
			+HIST(ctx->gm2, cube->r1, cube->g0, cube->b0)
			-HIST(ctx->gm2, cube->r0, cube->g1, cube->b1)
			+HIST(ctx->gm2, cube->r0, cube->g1, cube->b0)
			+HIST(ctx->gm2, cube->r0, cube->g0, cube->b1)
			-HIST(ctx->gm2, cube->r0, cube->g0, cube->b0);

	return xx - (dr*dr+dg*dg+db*db) / (ILfloat)(Vol(cube, ctx->wt));
}

/* We want to minimize the sum of the variances of two subBoxes.
 * The sum(c^2) terms can be ignored since their sum over both subBoxes
 * is the same (the sum for the whole Box) no matter where we split.
 * The remaining terms have a minus sign in the variance formula,
 * so we drop the minus sign and MAXIMIZE the sum of the two terms.
 */

static ILfloat Maximize(QuantContext *ctx, const Box *cube, ILubyte dir, ILint first, ILint last, ILint *cut,
				 ILint whole_r, ILint whole_g, ILint whole_b, ILint whole_w)
{
	ILint	half_r, half_g, half_b, half_w;
	ILint	base_r, base_g, base_b, base_w;
	ILint	i;
	ILfloat	temp, max;

	base_r = Bottom(cube, dir, ctx->mr);
	base_g = Bottom(cube, dir, ctx->mg);
	base_b = Bottom(cube, dir, ctx->mb);
	base_w = Bottom(cube, dir, ctx->wt);
	max = 0.0;
	*cut = -1;

	for (i = first; i < last; ++i) {
		half_r = base_r + Top(cube, dir, i, ctx->mr);
		half_g = base_g + Top(cube, dir, i, ctx->mg);
		half_b = base_b + Top(cube, dir, i, ctx->mb);
		half_w = base_w + Top(cube, dir, i, ctx->wt);
		// Now half_x is sum over lower half of Box, if split at i 
		if (half_w == 0) {  // subBox could be empty of pixels!
			continue;       // never split into an empty Box
		}
		else {
			temp = ((ILfloat)half_r*half_r + (ILfloat)half_g * half_g +
					(ILfloat)half_b*half_b) / half_w;
		}

		half_r = whole_r - half_r;
		half_g = whole_g - half_g;
		half_b = whole_b - half_b;
		half_w = whole_w - half_w;
		if (half_w == 0) {  // subBox could be empty of pixels!
			continue;       // never split into an empty Box
		}
		else {
			temp += ((ILfloat)half_r*half_r + (ILfloat)half_g * half_g +
					(ILfloat)half_b*half_b) / half_w;
		}

		if (temp > max) {
			max = temp;
			*cut = i;
		}
	}

	return max;
}


static ILint Cut(QuantContext *ctx, Box *set1, Box *set2)
{
	ILubyte dir;
	ILint cutr, cutg, cutb;
	ILfloat maxr, maxg, maxb;
	ILint whole_r, whole_g, whole_b, whole_w;

	whole_r = Vol(set1, ctx->mr);
	whole_g = Vol(set1, ctx->mg);
	whole_b = Vol(set1, ctx->mb);
	whole_w = Vol(set1, ctx->wt);

	maxr = Maximize(ctx, set1, RED,   set1->r0+1, set1->r1, &cutr, whole_r, whole_g, whole_b, whole_w);
	maxg = Maximize(ctx, set1, GREEN, set1->g0+1, set1->g1, &cutg, whole_r, whole_g, whole_b, whole_w);
	maxb = Maximize(ctx, set1, BLUE,  set1->b0+1, set1->b1, &cutb, whole_r, whole_g, whole_b, whole_w);

	if ((maxr >= maxg) && (maxr >= maxb)) {
		dir = RED;
		if (cutr < 0)
			return 0; // can't split the Box 
	}
	else if ((maxg >= maxr) && (maxg >= maxb))
		dir = GREEN;
	else
		dir = BLUE;

	set2->r1 = set1->r1;
	set2->g1 = set1->g1;
	set2->b1 = set1->b1;

	switch (dir)
	{
		case RED:
			set2->r0 = set1->r1 = cutr;
			set2->g0 = set1->g0;
			set2->b0 = set1->b0;
			break;
		case GREEN:
			set2->g0 = set1->g1 = cutg;
			set2->r0 = set1->r0;
			set2->b0 = set1->b0;
			break;
		case BLUE:
			set2->b0 = set1->b1 = cutb;
			set2->r0 = set1->r0;
			set2->g0 = set1->g0;
			break;
	}

	set1->vol = (set1->r1-set1->r0) * (set1->g1-set1->g0) * (set1->b1-set1->b0);
	set2->vol = (set2->r1-set2->r0) * (set2->g1-set2->g0) * (set2->b1-set2->b0);

	return 1;
}


static void Mark(Box *cube, ILubyte label, unsigned char *tag)
{
	ILint r, g, b;

	for (r = cube->r0 + 1; r <= cube->r1; r++) {
		for (g = cube->g0 + 1; g <= cube->g1; g++) {
			for (b = cube->b0 + 1; b <= cube->b1; b++) {
				tag[(r<<10) + (r<<6) + r + (g<<5) + g + b] = label;
			}
		}
	}
	return;
}


ILimage *iQuantizeImage(ILimage *Image, ILuint NumCols)
{
	Box		cube[MAXCOLOR];
	ILubyte	*tag = NULL;
	ILubyte	lut_r[MAXCOLOR], lut_g[MAXCOLOR], lut_b[MAXCOLOR];
	ILuint	next;
	ILint	weight;
	ILuint	i;
	ILuint k;
	ILfloat	vv[MAXCOLOR], temp;
	//ILint	color_num;
	ILubyte	*NewData = NULL, *Palette = NULL;
	ILimage	*TempImage = NULL, *NewImage = NULL;
	ILubyte	*Ir = NULL, *Ig = NULL, *Ib = NULL;

	ILuint num_alloced_colors; // number of colors we allocated space for in palette, as NumCols but will not be less than 256

	QuantContext ctx;

	num_alloced_colors = NumCols;
	if(num_alloced_colors<256) { 
		num_alloced_colors=256; 
	}

	TempImage = iConvertImage(Image, IL_RGB, IL_UNSIGNED_BYTE);

	if (TempImage == NULL)
		return NULL;

	ctx.buffer = Image->Data;
	ctx.WindW = ctx.Width = Image->Width;
	ctx.WindH = ctx.Height = Image->Height;
	ctx.WindD = ctx.Depth = Image->Depth;
	ctx.Comp = Image->Bpp;
	ctx.Qadd = NULL;

	//color_num = ImagePrecalculate(Image);

	NewData = (ILubyte*)ialloc(Image->Width * Image->Height * Image->Depth);
	Palette = (ILubyte*)ialloc(3 * num_alloced_colors);
	if (!NewData || !Palette) {
		ifree(NewData);
		ifree(Palette);
		return NULL;
	}

	ctx.size = ctx.Width * ctx.Height * ctx.Depth;

	Ir = (ILubyte*)ialloc(ctx.size);
	Ig = (ILubyte*)ialloc(ctx.size);
	Ib = (ILubyte*)ialloc(ctx.size);
	if (!Ir || !Ig || !Ib) {
		ifree(Ir);
		ifree(Ig);
		ifree(Ib);
		ifree(NewData);
		ifree(Palette);
		return NULL;
	}

	for (i = 0; i < ctx.size; i++) {
    Ir[i] = TempImage->Data[i * 3 + 0];
		Ig[i] = TempImage->Data[i * 3 + 1];
		Ib[i] = TempImage->Data[i * 3 + 2];
	}

	// Set new colors number
	ctx.K = NumCols;

	if (ctx.K <= 256) {
		// Begin Wu's color quantization algorithm

		// May have "leftovers" from a previous run.
    imemclear(ctx.gm2, 33 * 33 * 33 * sizeof(ILfloat));
    imemclear(ctx.wt,  33 * 33 * 33 * sizeof(ILint));
    imemclear(ctx.mr,  33 * 33 * 33 * sizeof(ILint));
    imemclear(ctx.mg,  33 * 33 * 33 * sizeof(ILint));
    imemclear(ctx.mb,  33 * 33 * 33 * sizeof(ILint));
                
		if (!Hist3d(&ctx, Ir, Ig, Ib, (ILint*)ctx.wt, (ILint*)ctx.mr, (ILint*)ctx.mg, (ILint*)ctx.mb, (ILfloat*)ctx.gm2))
			goto error_label;

		M3d((ILint*)ctx.wt, (ILint*)ctx.mr, (ILint*)ctx.mg, (ILint*)ctx.mb, (ILfloat*)ctx.gm2);

		cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
		cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;

		next = 0;
		for (i = 1; i < ctx.K; ++i) {
			if (Cut(&ctx, &cube[next], &cube[i])) { // volume test ensures we won't try to cut one-cell Box */
				vv[next] = (cube[next].vol>1) ? Var(&ctx, &cube[next]) : 0.0f;
				vv[i]    = (cube[i   ].vol>1) ? Var(&ctx, &cube[i   ]) : 0.0f;
			}
			else {
				vv[next] = 0.0;   // don't try to split this Box again
				i--;              // didn't create Box i
			}

			next = 0;
			temp = vv[0];

			for (k = 1; k <= i; ++k) {
				if (vv[k] > temp) {
					temp = vv[k]; next = k;
				}
			}
				
			if (temp <= 0.0) {
				ctx.K = i+1;
				// Only got K Boxes
				break;
			}
		}

		tag = (ILubyte*)ialloc(33 * 33 * 33 * sizeof(ILubyte));
		if (tag == NULL)
			goto error_label;

		for (k = 0; k < ctx.K; k++) {
			Mark(&cube[k], (ILubyte)k, tag);
			weight = Vol(&cube[k], ctx.wt);
			if (weight) {
				lut_r[k] = (ILubyte)(Vol(&cube[k], ctx.mr) / weight);
				lut_g[k] = (ILubyte)(Vol(&cube[k], ctx.mg) / weight);
				lut_b[k] = (ILubyte)(Vol(&cube[k], ctx.mb) / weight);
			}
			else {
				// Bogus Box
				lut_r[k] = lut_g[k] = lut_b[k] = 0;		
			}
		}

		for (i = 0; i < ctx.size; i++) {
			NewData[i] = tag[ctx.Qadd[i]];
		}
		ifree(tag);
		ifree(ctx.Qadd);

		for (k = 0; k < NumCols; k++) {
			Palette[k * 3]     = lut_b[k];
			Palette[k * 3 + 1] = lut_g[k];
			Palette[k * 3 + 2] = lut_r[k];
		}
	}
	else { // If colors more than 256
		// Begin Octree quantization
		//Quant_Octree(Image->Width, Image->Height, Image->Bpp, buffer2, NewData, Palette, K);
		iSetError(IL_INTERNAL_ERROR);  // Can't get much more specific than this.
		goto error_label;
	}

	ifree(Ig);
	ifree(Ib);
	ifree(Ir);
	iCloseImage(TempImage);

	NewImage = (ILimage*)icalloc(sizeof(ILimage), 1);
	if (NewImage == NULL) {
		return NULL;
	}
	iCopyImageAttr(NewImage, Image);
	NewImage->Bpp 				= 1;
	NewImage->Bps 				= Image->Width;
	NewImage->SizeOfPlane = NewImage->Bps * Image->Height;
	NewImage->SizeOfData 	= NewImage->SizeOfPlane;
	NewImage->Format 			= IL_COLOUR_INDEX;
	NewImage->Type 				= IL_UNSIGNED_BYTE;

	NewImage->Pal.Palette = Palette;
	NewImage->Pal.PalSize = 256 * 3;
	NewImage->Pal.PalType = IL_PAL_BGR24;
	NewImage->Data 				= NewData;

	return NewImage;

error_label:
	ifree(NewData);
	ifree(Palette);
	ifree(Ig);
	ifree(Ib);
	ifree(Ir);
	ifree(tag);
	ifree(ctx.Qadd);
	return NULL;
}
