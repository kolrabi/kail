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

static ILboolean iIsValidJascPal(SIO *io);
static ILboolean iSaveJascPal(ILimage *Image);
static ILboolean iLoadJascPal(ILimage *Image);

#define BUFFLEN 256
#define PALBPP	3

static ILboolean iIsValidJascPal(SIO *io) {
	ILuint 		First = SIOtell(io);
	ILubyte   Head[9];
	ILubyte   Read = SIOread(io, Head, 1, 9);

	SIOseek(io, First, IL_SEEK_SET);

	return Read == 9 && !memcmp(Head, "JASC-PAL\n", 9);
}

//! Loads a Paint Shop Pro formatted palette (.pal) file.
static ILboolean iLoadJascPal(ILimage *Image)
{
	ILuint NumColours, i, c;
	char Buff[BUFFLEN];

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;
	ILpal NewPal;
	imemclear(&NewPal, sizeof(NewPal));

	if ( !SIOgetw(io, Buff, BUFFLEN) 
	  || iCharStrICmp(Buff, "JASC-PAL") ) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if ( !SIOgetw(io, Buff, BUFFLEN) 
		|| iCharStrICmp(Buff, "0100") ) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!SIOgetw(io, Buff, BUFFLEN)) {
		iSetError(IL_FILE_READ_ERROR);
		return IL_FALSE;
	}

	NumColours = atoi(Buff);
	if (NumColours == 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	
	NewPal.PalSize = NumColours * PALBPP;
	NewPal.PalType = IL_PAL_RGB24;
	NewPal.Palette = (ILubyte*)ialloc(NumColours * PALBPP);
	if (NewPal.Palette == NULL) {
		return IL_FALSE;
	}

	for (i = 0; i < NumColours; i++) {
		for (c = 0; c < PALBPP; c++) {
			if (!SIOgetw(io, Buff, BUFFLEN)) {
				iSetError(IL_FILE_READ_ERROR);
				ifree(NewPal.Palette);
				return IL_FALSE;
			}

			NewPal.Palette[i * PALBPP + c] = atoi(Buff);
		}
	}

	if ( Image->Pal.Palette 
		&& Image->Pal.PalSize > 0 
		&& Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
	}
	Image->Pal = NewPal;

	return IL_TRUE;
}


//! Saves a Paint Shop Pro formatted palette (.pal) file.
static ILboolean iSaveJascPal(ILimage *Image) {
	ILuint	i, PalBpp, NumCols = ilGetInteger(IL_PALETTE_NUM_COLS);
	SIO *io = &Image->io;

	if (NumCols == 0 || NumCols > 256) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Create a copy of the current palette and convert it to RGB24 format.
	ILpal *ConvPal = iConvertPal(&Image->Pal, IL_PAL_RGB24);
	if (!ConvPal) {
		return IL_FALSE;
	}

	// Header needed on all .pal files
	SIOputs(io, "JASC-PAL\n0100\n256\n");

	PalBpp = ilGetBppPal(ConvPal->PalType);
	for (i = 0; i < ConvPal->PalSize; i += PalBpp) {
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "%d %d %d\n",
			ConvPal->Palette[i], ConvPal->Palette[i+1], ConvPal->Palette[i+2]);
		SIOputs(io, tmp);
	}

	NumCols = 256 - NumCols;
	for (i = 0; i < NumCols; i++) {
		SIOputs(io, "0 0 0\n");
	}

	ifree(ConvPal);
	return IL_TRUE;
}

ILconst_string iFormatExtsJASC_PAL[] = { 
	IL_TEXT("pal"),
  NULL 
};

ILformat iFormatJASC_PAL = { 
  .Validate = iIsValidJascPal, 
  .Load     = iLoadJascPal, 
  .Save     = iSaveJascPal,
  .Exts     = iFormatExtsJASC_PAL
};
