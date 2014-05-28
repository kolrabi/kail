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

static ILboolean iLoadPltPal(ILimage *Image);

// Hasn't been tested
//	@TODO: Test the thing!

//! Loads a .col palette file
// FIXME: restore / don't disturb image palette on failure

//! Loads an .plt palette file.
static ILboolean iLoadPltPal(ILimage *Image) {
	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;

	if (Image->Pal.Palette && Image->Pal.PalSize > 0 && Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
	}

	Image->Pal.PalSize = GetLittleUInt(&Image->io);
	if (Image->Pal.PalSize == 0) {
		return IL_FALSE;
	}

	Image->Pal.PalType = IL_PAL_RGB24;
	Image->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
	if (!Image->Pal.Palette) {
		return IL_FALSE;
	}

	if (SIOread(io, Image->Pal.Palette, Image->Pal.PalSize, 1) != 1) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
		return IL_FALSE;
	}

	return IL_TRUE;
}

ILconst_string iFormatExtsPLT_PAL[] = { 
	"plt",
  NULL 
};

ILformat iFormatPLT_PAL = { 
  .Validate = NULL, 
  .Load     = iLoadPltPal, 
  .Save     = NULL,
  .Exts     = iFormatExtsPLT_PAL
};
