/* NeuQuant Neural-Net Quantization Algorithm
 * ------------------------------------------
 *
 * Copyright (c) 1994 Anthony Dekker
 *
 * NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994.
 * See "Kohonen neural networks for optimal colour quantization"
 * in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
 * for a discussion of the algorithm.
 * See also  http://www.acm.org/~dekker/NEUQUANT.HTML
 *
 * Any party obtaining a copy of these files from the author, directly or
 * indirectly, is granted, free of charge, a full and unrestricted irrevocable,
 * world-wide, paid up, royalty-free, nonexclusive right and license to deal
 * in this software and documentation files (the "Software"), including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 */

//-----------------------------------------------------------------------------
//
// ImageLib Sources
// by Denton Woods
// Last modified: 2014-05-27 by Björn Paetzel
//
// Filename: src-IL/src/il_neuquant.c
//
// Description: Heavily modified by Denton Woods.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"


// four primes near 500 - assume no image has a length so large
// that it is divisible by all four primes
#define prime1      499
#define prime2      491
#define prime3      487
#define prime4      503

// Network Definitions
// -------------------
   
#define netsize           256         // number of colours used
#define maxnetpos(ctx)      ((ctx)->netsizethink-1)
#define netbiasshift        4         // bias for colour values
#define ncycles           100         // no. of learning cycles

// defs for freq and bias
#define intbiasshift       16         // bias for fractions
#define intbias             (((ILint) 1)<<intbiasshift)
#define gammashift         10         // gamma = 1024
//#define gamma               (((ILint) 1)<<gammashift)
#define betashift          10
#define beta                (intbias>>betashift)// beta = 1/1024
#define betagamma           (intbias<<(gammashift-betashift))

// defs for decreasing radius factor
#define initrad             (netsize>>3)    // for 256 cols, radius starts
#define radiusbiasshift     6         // at 32.0 biased by 6 bits
#define radiusbias          (((ILint) 1)<<radiusbiasshift)
#define initradius          (initrad*radiusbias)  // and decreases by a
#define radiusdec          30            // factor of 1/30 each cycle

// defs for decreasing alpha factor
#define alphabiasshift     10            // alpha starts at 1.0
#define initalpha           (((ILint) 1)<<alphabiasshift)

// radbias and alpharadbias used for radpower calculation
#define radbiasshift        8
#define radbias             (((ILint) 1)<<radbiasshift)
#define alpharadbshift      (alphabiasshift+radbiasshift)
#define alpharadbias        (((ILint) 1)<<alpharadbshift)


// Types and Global Variables
// --------------------------

typedef struct {
  ILint r,g,b;
  ILuint index;
} NeuPixel;

typedef struct {
  const ILubyte * thepicture;         // the input image itself
  ILuint          lengthcount;        // lengthcount = H*W*3
  ILuint          samplefac;          // sampling factor 1..30
  NeuPixel        network[netsize];   // the network itself
  ILuint          netindex[256];      // for network lookup - really 256
  ILint           bias [netsize];     // bias and freq arrays for learning
  ILint           freq [netsize];
  ILint           radpower[initrad];  // radpower for precomputation

  ILuint          netsizethink;       // number of colors we want to reduce to, 2-256
  ILint           alphadec;           // biased by 10 bits
} NeuQuantContext;

// Initialise network in range (0,0,0) to (255,255,255) and set parameters
// -----------------------------------------------------------------------

static void NeuQuantInitNet(NeuQuantContext *ctx, const ILubyte *thepic, ILuint len, ILuint sample, ILuint NumCols) {
  ILuint i;

  ctx->thepicture   = thepic;
  ctx->lengthcount  = len;
  ctx->samplefac    = sample;
  ctx->netsizethink = NumCols;
  ctx->alphadec     = 30 + ((ctx->samplefac-1)/3);
  
  for (i=0; i<ctx->netsizethink; i++) {
    ctx->network[i].r =
    ctx->network[i].g =
    ctx->network[i].b = (ILint)((i << (netbiasshift+8))/netsize);
    ctx->freq[i]      = intbias / ctx->netsizethink; // 1/netsize
    ctx->bias[i]      = 0;
  }
}

  
// Unbias network to give byte values 0..255 and record position i to prepare for sort
// -----------------------------------------------------------------------------------

static void NeuQuantUnbiasNet(NeuQuantContext *ctx)
{
  ILuint i;

  for (i=0; i<ctx->netsizethink; i++) {
    ctx->network[i].r >>= netbiasshift;
    ctx->network[i].g >>= netbiasshift;
    ctx->network[i].b >>= netbiasshift;
    ctx->network[i].index = i;      // record colour no
  }
}


// Insertion sort of network and building of netindex[0..255] (to do after unbias)
// -------------------------------------------------------------------------------

static void NeuQuantBuild(NeuQuantContext *ctx)
{
  ILuint    i, j, smallpos;
  ILuint    startpos;
  ILint     previouscol, smallval;
  NeuPixel *p, *q;

  previouscol = 0;
  startpos = 0;
  for (i=0; i<ctx->netsizethink; i++) {
    p         = &ctx->network[i];
    smallpos  = i;
    smallval  = p->g;      // index on g

    // find smallest in i..netsize-1
    for (j=i+1; j<ctx->netsizethink; j++) {
      q = &ctx->network[j];
      if (q->g < smallval) {  // index on g
        smallpos = j;
        smallval = q->g;  // index on g
      }
    }

    q = &ctx->network[smallpos];

    // swap p (i) and q (smallpos) entries
    if (i != smallpos) {
      NeuPixel tmp = *q;
      *q = *p;
      *p = tmp;
    }

    // smallval entry is now in position i
    if (smallval != previouscol) {
      ctx->netindex[previouscol] = (startpos+i)>>1;
      for (previouscol++; previouscol<smallval; previouscol++) 
        ctx->netindex[previouscol] = i;

      previouscol = smallval;
      startpos    = i;
    }
  }

  ctx->netindex[previouscol] = (startpos+maxnetpos(ctx))>>1;
  for (previouscol++; previouscol<256; previouscol++) 
    ctx->netindex[previouscol] = maxnetpos(ctx); // really 256
}


// Search for BGR values 0..255 (after net is unbiased) and return colour index
// ----------------------------------------------------------------------------

static ILubyte NeuQuantFindColour(NeuQuantContext *ctx, ILint b, ILint g, ILint r)
{ 
  ILuint i,j;
  ILint dist, bestd;
  NeuPixel *p;
  ILuint best;

  bestd = 1000;   // biggest possible dist is 256*3
  best  = 0;
  i     = ctx->netindex[g];  // index on g
  j     = i-1;               // start at netindex[g] and work outwards

  while ((i<ctx->netsizethink) || (j<ctx->netsizethink)) {
    if (i<ctx->netsizethink) {
      p     = &ctx->network[i];
      dist  = abs(p->g - g); 
      dist += abs(p->b - b);
      dist += abs(p->r - r);

      if (dist < bestd) {
        bestd = dist; 
        best  = p->index;
      }

      i++;
    }

    if (j<ctx->netsizethink) {
      p = &ctx->network[j];
      dist  = abs(p->g - g); 
      dist += abs(p->b - b);
      dist += abs(p->r - r);

      if (dist < bestd) {
        bestd = dist; 
        best  = p->index;
      }

      j--;
    }
  }
  return (ILubyte)best;
}


// Search for biased BGR values
// ----------------------------

static ILuint NeuQuantContest(NeuQuantContext *ctx, ILint b, ILint g, ILint r)
{
  // finds closest neuron (min dist) and updates freq
  // finds best neuron (min dist-bias) and returns position
  // for frequently chosen neurons, freq[i] is high and bias[i] is negative
  // bias[i] = gamma*((1/netsize)-freq[i])

  ILuint i, bestpos, bestbiaspos;
  ILint  dist, biasdist, betafreq;
  ILint  bestd, bestbiasd;
  ILint  *p, *f;

  bestd       = 0x7FFFFFFF; // ~(((ILint) 1)<<31);
  bestbiasd   = bestd;
  bestpos     = netsize-1;
  bestbiaspos = bestpos;
  p           = ctx->bias;
  f           = ctx->freq;

  for (i=0; i<ctx->netsizethink; i++) {
    NeuPixel *n = &ctx->network[i];
    dist  = abs(n->b - b);
    dist += abs(n->g - g);
    dist += abs(n->r - r);

    if (dist<bestd) {
      bestd = dist; 
      bestpos = i;
    }

    biasdist = dist - ((*p)>>(intbiasshift-netbiasshift));

    if (biasdist<bestbiasd) {
      bestbiasd = biasdist; 
      bestbiaspos = i;
    }

    betafreq = (*f >> betashift);
    *f++ -= betafreq;
    *p++ += (betafreq<<gammashift);
  }
  ctx->freq[bestpos] += beta;
  ctx->bias[bestpos] -= betagamma;

  return bestbiaspos;
}


// Move neuron i towards biased (b,g,r) by factor alpha
// ----------------------------------------------------

static void NeuQuantAlterNeuron(NeuQuantContext *ctx, ILint alpha, ILuint i, ILint b, ILint g, ILint r)
{
  NeuPixel *n = &ctx->network[i];       // alter hit neuron
  n->r -= (alpha*(n->r - r)) / initalpha;
  n->g -= (alpha*(n->g - g)) / initalpha;
  n->b -= (alpha*(n->b - b)) / initalpha;
}


// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
// ---------------------------------------------------------------------------------

static void NeuQuantAlterNeighbours(NeuQuantContext *ctx, ILuint rad, ILuint i, ILint b, ILint g, ILint r)
{
  ILint   a;
  ILuint  k, lo;
  ILuint  j, hi;
  NeuPixel *p;
  ILint   *q;

  lo = 0;   
  if (i > rad) {
    lo = i - rad;
  }

  hi = ctx->netsizethink;
  if (i + rad < ctx->netsizethink) {
    hi = i + rad;   
  } 

  j = i+1;
  k = i-1;
  q = ctx->radpower;

  while ((j<hi) || (k>lo && k<ctx->netsizethink)) {
    a = (*(++q));
    if (j<hi) {
      p = &ctx->network[j];
      p->b -= (a*(p->b - b)) / alpharadbias;
      p->g -= (a*(p->g - g)) / alpharadbias;
      p->r -= (a*(p->r - r)) / alpharadbias;
      j++;
    }
    if (k>=lo && k<ctx->netsizethink) {
      p = &ctx->network[k];
      p->b -= (a*(p->b - b)) / alpharadbias;
      p->g -= (a*(p->g - g)) / alpharadbias;
      p->r -= (a*(p->r - r)) / alpharadbias;
      k--;
    }
  }
}


// Main Learning Loop
// ------------------

static void NeuQuantLearn(NeuQuantContext *ctx)
{
  ILint           b, g, r;
  ILuint          i, j, rad;
  ILint           alpha;
  ILuint          radius, step, delta, samplepixels;
  const ILubyte * p;
  const ILubyte * lim;

  p             = ctx->thepicture;
  lim           = ctx->thepicture + ctx->lengthcount;
  samplepixels  = ctx->lengthcount/(3*ctx->samplefac);
  delta         = samplepixels/ncycles;
  alpha         = initalpha;
  radius        = initradius;
  rad           = radius >> radiusbiasshift;

  for (i=0; i<rad; i++) 
    ctx->radpower[i] = alpha*(ILint)(((rad*rad - i*i)*radbias)/(rad*rad));
  
  if      ((ctx->lengthcount%prime1) != 0) step = 3*prime1;
  else if ((ctx->lengthcount%prime2) != 0) step = 3*prime2;
  else if ((ctx->lengthcount%prime3) != 0) step = 3*prime3;
  else                                     step = 3*prime4;
  
  // beginning 1D learning: initial radius=rad
  for (i=0; i<samplepixels; i++) {
    // find neuron closest to pixel color (and count frequency)
    b = p[0] << netbiasshift;
    g = p[1] << netbiasshift;
    r = p[2] << netbiasshift;
    j = NeuQuantContest(ctx, b,g,r);

    // approximate pixel color succesively
    NeuQuantAlterNeuron(ctx, alpha, j, b,g,r);
    if (rad) {
      NeuQuantAlterNeighbours(ctx, rad, j, b,g,r);   // alter neighbours
    }

    // select next pixel, wrap around
    p += step;
    if (p >= lim) {
      p -= ctx->lengthcount;
    }
  
    // update alpha, radius every delta samples
    if (i%delta == 0) { 
      alpha  -= alpha  /  ctx->alphadec;
      radius -= radius /  radiusdec;
      rad     = radius >> radiusbiasshift;

      for (j=0; j<rad; j++) 
        ctx->radpower[j] = alpha*(ILint)(((rad*rad - j*j)*radbias)/(rad*rad));
    }
  }
  // finished 1D learning: final alpha=alpha/initalpha;
}


ILimage *iNeuQuant(ILimage *Image, ILuint NumCols)
{
  ILimage *TempImage, *NewImage;
  ILuint  sample, i, j;
  NeuQuantContext ctx;

  TempImage = iConvertImage(Image, IL_BGR, IL_UNSIGNED_BYTE);
  sample    = (ILuint)ilGetInteger(IL_NEU_QUANT_SAMPLE);

  if (TempImage == NULL)
    return NULL;

  NeuQuantInitNet(&ctx, TempImage->Data, TempImage->SizeOfData, sample, NumCols);
  NeuQuantLearn(&ctx);
  NeuQuantUnbiasNet(&ctx);

  NewImage = (ILimage*)icalloc(sizeof(ILimage), 1);
  if (NewImage == NULL) {
    iCloseImage(TempImage);
    return NULL;
  }
  NewImage->Data = (ILubyte*)ialloc(TempImage->SizeOfData / 3);
  if (NewImage->Data == NULL) {
    iCloseImage(TempImage);
    ifree(NewImage);
    return NULL;
  }
  iCopyImageAttr(NewImage, Image);

  NewImage->Bpp         = 1;
  NewImage->Bps         = Image->Width;
  NewImage->SizeOfPlane = NewImage->Bps * Image->Height;
  NewImage->SizeOfData  = NewImage->SizeOfPlane;
  NewImage->Format      = IL_COLOUR_INDEX;
  NewImage->Type        = IL_UNSIGNED_BYTE;

  NewImage->Pal.PalSize = ctx.netsizethink * 3;
  NewImage->Pal.PalType = IL_PAL_BGR24;
  NewImage->Pal.Palette = (ILubyte*)ialloc(256*3);
  if (NewImage->Pal.Palette == NULL) {
    iCloseImage(TempImage);
    iCloseImage(NewImage);
    return NULL;
  }

  for (i = 0, j = 0; i < ctx.netsizethink; i++, j += 3) {
    NewImage->Pal.Palette[j  ] = (ILubyte)ctx.network[i].b;
    NewImage->Pal.Palette[j+1] = (ILubyte)ctx.network[i].g;
    NewImage->Pal.Palette[j+2] = (ILubyte)ctx.network[i].r;
  }

  NeuQuantBuild(&ctx);
  for (i = 0, j = 0; j < TempImage->SizeOfData; i++, j += 3) {
    NewImage->Data[i] = NeuQuantFindColour(&ctx, 
      TempImage->Data[j], TempImage->Data[j+1], TempImage->Data[j+2]);
  }

  iCloseImage(TempImage);

  return NewImage;
}
