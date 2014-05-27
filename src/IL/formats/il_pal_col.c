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

static ILboolean iIsValidColPal(SIO *io);
static ILboolean iLoadColPal(ILimage *Image);

// TODO: check if i got the endianness right

ILboolean iIsValidColPal(SIO *io) {
	ILuint 		First = SIOtell(io);
	ILushort  Version = GetLittleUShort(io);
	SIOseek(io, First, IL_SEEK_SET);
	return Version == 0xB123;
}


// Hasn't been tested
//	@TODO: Test the thing!

//! Loads a .col palette file
// FIXME: restore / don't disturb image palette on failure
static ILboolean iLoadColPal(ILimage *Image)
{
	ILuint		RealFileSize, FileSize;
	ILushort	Version;

	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;

	if (Image->Pal.Palette && Image->Pal.PalSize > 0 && Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
	}

	SIOseek(io, 0, IL_SEEK_END);
	RealFileSize = SIOtell(io);
	SIOseek(io, 0, IL_SEEK_SET);

	if (RealFileSize > 768) {  // has a header
		FileSize = GetLittleUInt(io);
		if ((FileSize - 8) % 3 != 0) {  // check to make sure an even multiple of 3!
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		Version = GetLittleUShort(io);
		if (Version != 0xB123) {
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		Version = GetLittleUShort(io);
		if (Version != 0) {
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	Image->Pal.Palette = (ILubyte*)ialloc(768);
	if (Image->Pal.Palette == NULL) {
		return IL_FALSE;
	}

	if (SIOread(io, Image->Pal.Palette, 1, 768) != 768) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
		return IL_FALSE;
	}

	Image->Pal.PalSize = 768;
	Image->Pal.PalType = IL_PAL_RGB24;

	return IL_TRUE;
}


ILconst_string iFormatExtsCOL_PAL[] = { 
	"col",
  NULL 
};

ILformat iFormatCOL_PAL = { 
  .Validate = iIsValidColPal, 
  .Load     = iLoadColPal, 
  .Save     = NULL,
  .Exts     = iFormatExtsCOL_PAL
};
