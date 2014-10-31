//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_pal.c
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_pal.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

// Hasn't been tested
//	@TODO: Test the thing!

//! Loads an .plt palette file.
static ILboolean iLoadPltPal(ILimage *Image) {
	SIO *io;
	ILpal NewPal;
	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;
	imemclear(&NewPal, sizeof(NewPal));

	NewPal.PalSize = GetLittleUInt(io);
	if (NewPal.PalSize == 0) {
		return IL_FALSE;
	}

	NewPal.PalType = IL_PAL_RGB24;
	NewPal.Palette = (ILubyte*)ialloc(NewPal.PalSize);
	if (!NewPal.Palette) {
		return IL_FALSE;
	}

	if (SIOread(io, NewPal.Palette, NewPal.PalSize, 1) != 1) {
		ifree(NewPal.Palette);
		return IL_FALSE;
	}

	if ( Image->Pal.Palette 
		&& Image->Pal.PalSize > 0 
		&& Image->Pal.PalType != IL_PAL_NONE ) {
		ifree(Image->Pal.Palette);
	}
	Image->Pal = NewPal;
	return IL_TRUE;
}

static ILconst_string iFormatExtsPLT_PAL[] = { 
	IL_TEXT("plt"),
  NULL 
};

ILformat iFormatPLT_PAL = { 
  /* .Validate = */ NULL, 
  /* .Load     = */ iLoadPltPal, 
  /* .Save     = */ NULL,
  /* .Exts     = */ iFormatExtsPLT_PAL
};
