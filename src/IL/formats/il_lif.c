//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_lif.c
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_LIF
#include "il_lif.h"

static ILboolean iCheckLif(LIF_HEAD *Header);

// Internal function used to get the Lif header from the current file.
static ILboolean iGetLifHead(SIO *io, LIF_HEAD *Header) {
	if (SIOread(io, Header, sizeof(*Header), 1) != 1)
		return IL_FALSE;

	UInt(&Header->Version);
	UInt(&Header->Flags);
	UInt(&Header->Width);
	UInt(&Header->Height);
	UInt(&Header->PaletteCRC);
	UInt(&Header->ImageCRC);
	UInt(&Header->PalOffset);
	UInt(&Header->TeamEffect0);
	UInt(&Header->TeamEffect1);

	return IL_TRUE;
}

// Internal function to get the header and check it.
static ILboolean iIsValidLif(SIO *io) {
	ILuint    Pos = SIOtell(io);
	LIF_HEAD	Head;

	if (!iGetLifHead(io, &Head)) {
		SIOseek(io, Pos, IL_SEEK_SET);
		return IL_FALSE;
	}
	SIOseek(io, Pos, IL_SEEK_SET);

	return iCheckLif(&Head);
}

// Internal function used to check if the HEADER is a valid Lif header.
static ILboolean iCheckLif(LIF_HEAD *Header) {
	if (Header->Version != 260 || Header->Flags != 50)
		return IL_FALSE;

	if (memcmp(Header->Id, "Willy 7", 7))
		return IL_FALSE;

	return IL_TRUE;
}

static ILboolean iLoadLifInternal(ILimage *Image) {
	LIF_HEAD	LifHead;
	ILuint		i;

	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;
	
	if (!iGetLifHead(io, &LifHead))
		return IL_FALSE;

	if (!ilTexImage_(Image, LifHead.Width, LifHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	Image->Pal.Palette = (ILubyte*)ialloc(1024);

	if (Image->Pal.Palette == NULL)
		return IL_FALSE;

	Image->Pal.PalSize = 1024;
	Image->Pal.PalType = IL_PAL_RGBA32;

	if (SIOread(io, Image->Data, LifHead.Width * LifHead.Height, 1) != 1)
		return IL_FALSE;

	if (SIOread(io, Image->Pal.Palette, 1, 1024) != 1024)
		return IL_FALSE;

	// Each data offset is offset by -1, so we add one.
	for (i = 0; i < Image->SizeOfData; i++) {
		Image->Data[i]++;
	}

	return ilFixImage();
}

ILconst_string iFormatExtsLIF[] = { 
  IL_TEXT("lif"), 
  NULL 
};

ILformat iFormatLIF = { 
  .Validate = iIsValidLif, 
  .Load     = iLoadLifInternal, 
  .Save     = NULL, 
  .Exts     = iFormatExtsLIF
};

#endif//IL_NO_LIF
