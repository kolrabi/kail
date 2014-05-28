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

static ILboolean iIsValidActPal(SIO *io);
static ILboolean iLoadActPal(ILimage *Image);

// TODO: check if i got the endianness right

ILboolean iIsValidActPal(SIO *io) {
	ILuint 		First = SIOtell(io);
	SIOseek(io, 0, IL_SEEK_END);
	ILuint    Last  = SIOtell(io);
	SIOseek(io, First, IL_SEEK_SET);
	return Last == 768; 
}

// Hasn't been tested
//	@TODO: Test the thing!

//! Loads an .act palette file.
static ILboolean iLoadActPal(ILimage *Image) {
 	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;
	ILpal NewPal;
	imemclear(&NewPal, sizeof(NewPal));

	NewPal.PalType = IL_PAL_RGB24;
	NewPal.PalSize = 768;
	NewPal.Palette = (ILubyte*)ialloc(768);
	if (!NewPal.Palette) {
		return IL_FALSE;
	}

	if (SIOread(io, NewPal.Palette, 1, 768) != 768) {
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

ILconst_string iFormatExtsACT_PAL[] = { 
	IL_TEXT("act"),
  NULL 
};

ILformat iFormatACT_PAL = { 
  .Validate = iIsValidActPal, 
  .Load     = iLoadActPal, 
  .Save     = NULL,
  .Exts     = iFormatExtsACT_PAL
};
