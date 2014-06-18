//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pcd.c
//
// Description: Reads from a Kodak PhotoCD (.pcd) file.
//		Note:  The code here is sloppy - I had to convert it from Pascal,
//				which I've never even attempted to read before...enjoy! =)
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PCD
#include "il_manip.h"
#include "il_color.h"

static ILboolean iLoadPcdInternal(ILimage* image) {
	ILubyte	VertOrientation;
	ILuint	Width, Height, i, Total, x, CurPos = 0;
	ILubyte	*Y1=NULL, *Y2=NULL, *CbCr=NULL, r = 0, g = 0, b = 0;
    SIO *io;
	ILuint Start;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;
	Start = SIOtell(io);

	SIOseek(io, 72, IL_SEEK_CUR);
	if (SIOread(io, &VertOrientation, 1, 1) != 1)
		return IL_FALSE;
	SIOseek(io, Start, IL_SEEK_SET);

	switch (iGetInt(IL_PCD_PICNUM)) {
		case 0:
			SIOseek(io, 0x02000, IL_SEEK_CUR);
			Width  = 192;
			Height = 128;
			break;
		case 1:
			SIOseek(io, 0x0b800, IL_SEEK_CUR);
			Width  = 384;
			Height = 256;
			break;
		case 2:
			SIOseek(io, 0x30000, IL_SEEK_CUR);
			Width  = 768;
			Height = 512;
			break;
		default:
			iSetError(IL_INVALID_PARAM);
			return IL_FALSE;
	}

	if (SIOeof(io))  // Supposed to have data here.
		return IL_FALSE;

	Y1   = (ILubyte*)ialloc(Width);
	Y2   = (ILubyte*)ialloc(Width);
	CbCr = (ILubyte*)ialloc(Width);

	if (Y1 == NULL || Y2 == NULL || CbCr == NULL) {
		ifree(Y1);
		ifree(Y2);
		ifree(CbCr);
		return IL_FALSE;
	}

	if (!iTexImage(image, Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	Total = Height >> 1;
	for (i = 0; i < Total; i++) {
		SIOread(io, Y1, 1, Width);
		SIOread(io, Y2, 1, Width);

		if (SIOread(io, CbCr, 1, Width) != Width) {  // Only really need to check the last one.
			ifree(Y1);
			ifree(Y2);
			ifree(CbCr);
			return IL_FALSE;
		}

		for (x = 0; x < Width; x++) {
			iYCbCr2RGB(Y1[x], CbCr[x / 2], CbCr[(Width / 2) + (x / 2)], &r, &g, &b);
			image->Data[CurPos++] = r;
			image->Data[CurPos++] = g;
			image->Data[CurPos++] = b;
		}

		for (x = 0; x < Width; x++) {
			iYCbCr2RGB(Y2[x], CbCr[x / 2], CbCr[(Width / 2) + (x / 2)], &r, &g, &b);
			image->Data[CurPos++] = r;
			image->Data[CurPos++] = g;
			image->Data[CurPos++] = b;
		}
	}

	ifree(Y1);
	ifree(Y2);
	ifree(CbCr);

	// Not sure how it is...the documentation is hard to understand
	if ((VertOrientation & 0x3F) != 8) {
		image->Origin = IL_ORIGIN_LOWER_LEFT;
	}	else {
		image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	return IL_TRUE;
}

ILconst_string iFormatExtsPCD[] = { 
  IL_TEXT("pcd"), 
  NULL 
};

ILformat iFormatPCD = { 
  /* .Validate = */ NULL, // TODO: iIsValidPCD, 
  /* .Load     = */ iLoadPcdInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsPCD
};

#endif//IL_NO_PCD
