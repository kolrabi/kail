//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_wal.c
//
// Description: Loads a Quake .wal texture.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_WAL
#include "il_manip.h"
#include "il_q2pal.h"

#include "pack_push.h"
typedef struct WALHEAD
{
	ILbyte	FileName[32];	// Image name
	ILuint	Width;			// Width of first image
	ILuint	Height;			// Height of first image
	ILuint	Offsets[4];		// Offsets to image data
	ILbyte	AnimName[32];	// Name of next frame
	ILuint	Flags;			// ??
	ILuint	Contents;		// ??
	ILuint	Value;			// ??
} WALHEAD;
#include "pack_pop.h"

static ILboolean iLoadWalInternal(ILimage *);



static ILboolean iLoadWalInternal(ILimage *Image)
{
	WALHEAD	Header;
	ILimage	*Mipmaps[3], *CurImage;
	ILuint	i, NewW, NewH;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	CurImage = Image;

	SIO *io = &Image->io;

	// Read header
	SIOread(io, &Header.FileName, 1, 32);
	Header.Width 	= GetLittleUInt(io);
	Header.Height = GetLittleUInt(io);

	for (i = 0; i < 4; i++)
		Header.Offsets[i] = GetLittleUInt(io);

	SIOread(io, Header.AnimName, 1, 32);
	Header.Flags = GetLittleUInt(io);
	Header.Contents = GetLittleUInt(io);
	Header.Value = GetLittleUInt(io);

	if (!ilTexImage_(Image, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < 3; i++) {
		Mipmaps[i] = (ILimage*)icalloc(sizeof(ILimage), 1);
		if (Mipmaps[i] == NULL)
			goto cleanup_error;
		Mipmaps[i]->Pal.Palette = (ILubyte*)ialloc(768);
		if (Mipmaps[i]->Pal.Palette == NULL)
			goto cleanup_error;
		memcpy(Mipmaps[i]->Pal.Palette, ilDefaultQ2Pal, 768);
		Mipmaps[i]->Pal.PalType = IL_PAL_RGB24;
	}

	NewW = Header.Width;
	NewH = Header.Height;
	for (i = 0; i < 3; i++) {
		NewW /= 2;
		NewH /= 2;
		Image = Mipmaps[i];
		if (!ilTexImage_(Image, NewW, NewH, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
			goto cleanup_error;
		// Don't set until now so ilTexImage won't get rid of the palette.
		Mipmaps[i]->Pal.PalSize = 768;
		Mipmaps[i]->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	Image = CurImage;
	ilCloseImage(Image->Mipmaps);
	Image->Mipmaps = Mipmaps[0];
	Mipmaps[0]->Mipmaps = Mipmaps[1];
	Mipmaps[1]->Mipmaps = Mipmaps[2];

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	if (Image->Pal.Palette && Image->Pal.PalSize && Image->Pal.PalType != IL_PAL_NONE)
		ifree(Image->Pal.Palette);
	Image->Pal.Palette = (ILubyte*)ialloc(768);
	if (Image->Pal.Palette == NULL)
		goto cleanup_error;

	Image->Pal.PalSize = 768;
	Image->Pal.PalType = IL_PAL_RGB24;
	memcpy(Image->Pal.Palette, ilDefaultQ2Pal, 768);

	SIOseek(io, Header.Offsets[0], IL_SEEK_SET);
	if (SIOread(io, Image->Data, Header.Width * Header.Height, 1) != 1)
		goto cleanup_error;

	for (i = 0; i < 3; i++) {
		SIOseek(io, Header.Offsets[i+1], IL_SEEK_SET);
		if (SIOread(io, Mipmaps[i]->Data, Mipmaps[i]->Width * Mipmaps[i]->Height, 1) != 1)
			goto cleanup_error;
	}

	// Fixes all images, even mipmaps.
	return IL_TRUE;

cleanup_error:
	for (i = 0; i < 3; i++) {
		ilCloseImage(Mipmaps[i]);
	}
	return IL_FALSE;
}


ILconst_string iFormatExtsWAL[] = { 
	IL_TEXT("wal"), 
	NULL 
};

ILformat iFormatWAL = { 
	.Validate = NULL, 
	.Load     = iLoadWalInternal, 
	.Save     = NULL, 
	.Exts     = iFormatExtsWAL
};

#endif//IL_NO_WAL
