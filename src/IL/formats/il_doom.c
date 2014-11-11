//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 2014-05-22 by Björn Paetzel
//
// Filename: src-IL/src/il_doom.c
//
// Description: Reads Doom textures and flats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DOOM
#include "il_manip.h"
#include "il_pal.h"
#include "il_doompal.h"

#include "pack_push.h"
typedef struct {
	ILushort width, height;
	ILuint graphic_header;
} DOOM_HEAD;
#include "pack_pop.h"

//
// READ A DOOM IMAGE
//

// From the DTE sources (mostly by Denton Woods with corrections by Randy Heit)
static ILboolean iLoadDoomInternal(ILimage *Image)
{
	SIO *io;
	ILuint first_pos;
	DOOM_HEAD head;

	ILushort column_loop;

	ILuint	i;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;
	first_pos = SIOtell(io);

	if (SIOread(io, &head, 1, sizeof(head)) != sizeof(head)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	UShort(&head.width);
	UShort(&head.height);
	UInt  (&head.graphic_header);

	if (!iTexImage(Image, head.width, head.height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	Image->Pal.Palette = (ILubyte*)ialloc(IL_DOOMPAL_SIZE);
	if (Image->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	Image->Pal.PalSize = IL_DOOMPAL_SIZE;
	Image->Pal.PalType = IL_PAL_RGB24;
	memcpy(Image->Pal.Palette, ilDefaultDoomPal, IL_DOOMPAL_SIZE);

	// 247 is always the transparent colour (usually cyan)
	memset(Image->Data, 247, Image->SizeOfData);
	for (column_loop = 0; column_loop < head.width; column_loop++) {
		ILuint column_offset;
        ILuint pointer_position;

		if (SIOread(io, &column_offset, 1, sizeof(column_offset)) != sizeof(column_offset))
			return IL_FALSE;

		UInt(&column_offset);
		
		pointer_position = SIOtell(io);
		SIOseek(io, first_pos + column_offset, IL_SEEK_SET);

		while (1) {
			ILubyte topdelta;
			ILubyte length;
			ILubyte post;
			ILushort	row_loop;

			if (SIOread(io, &topdelta, 1, 1) != 1)
				return IL_FALSE;

			if (topdelta == 255)
				break;

			if (SIOread(io, &length, 1, 1) != 1)
				return IL_FALSE;

			if (SIOread(io, &post, 1, 1) != 1)
				return IL_FALSE; // Skip extra byte for scaling

			for (row_loop = 0; row_loop < length; row_loop++) {
				if (SIOread(io, &post, 1, 1) != 1)
					return IL_FALSE;

				if (row_loop + topdelta < head.height)
					Image->Data[(row_loop+topdelta) * head.width + column_loop] = post;
			}
			if (SIOread(io, &post, 1, 1) != 1) // Skip extra scaling byte
				return IL_FALSE;
		}

		SIOseek(io, pointer_position, IL_SEEK_SET);
	}

	// Converts palette entry 247 (cyan) to transparent.
	if (iIsEnabled(IL_CONV_PAL)) {
		ILubyte	*NewData = (ILubyte*)ialloc(Image->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < Image->SizeOfData; i++) {
			NewData[i * 4 + 0] = Image->Pal.Palette[Image->Data[i] + 0];
			NewData[i * 4 + 1] = Image->Pal.Palette[Image->Data[i] + 1];
			NewData[i * 4 + 2] = Image->Pal.Palette[Image->Data[i] + 2];
			NewData[i * 4 + 3] = Image->Data[i] != 247 ? 255 : 0;
		}

		if (!iTexImage(Image, Image->Width, Image->Height, Image->Depth, 4, IL_RGBA, Image->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}

		Image->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	return IL_TRUE;
}


//
// READ A DOOM FLAT
//

// Basically just ireads 4096 bytes and copies the palette
static ILboolean iLoadDoomFlatInternal(ILimage *Image)
{
	ILuint	i;
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	if (!iTexImage(Image, 64, 64, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	Image->Pal.Palette = (ILubyte*)ialloc(IL_DOOMPAL_SIZE);
	if (Image->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	Image->Pal.PalSize = IL_DOOMPAL_SIZE;
	Image->Pal.PalType = IL_PAL_RGB24;
	memcpy(Image->Pal.Palette, ilDefaultDoomPal, IL_DOOMPAL_SIZE);

	if (SIOread(io, Image->Data, 1, 4096) != 4096)
		return IL_FALSE;

	if (iIsEnabled(IL_CONV_PAL)) {
			ILubyte	*NewData = (ILubyte*)ialloc(Image->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < Image->SizeOfData; i++) {
			NewData[i * 4 + 0] = Image->Pal.Palette[Image->Data[i] + 0];
			NewData[i * 4 + 1] = Image->Pal.Palette[Image->Data[i] + 1];
			NewData[i * 4 + 2] = Image->Pal.Palette[Image->Data[i] + 2];
			NewData[i * 4 + 3] = Image->Data[i] != 247 ? 255 : 0;
		}

		if (!iTexImage(Image, Image->Width, Image->Height, Image->Depth, 4, IL_RGBA, Image->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	return IL_TRUE;
}

static ILconst_string iFormatExtsDOOM[] = { 
	NULL 
};

ILformat iFormatDOOM = { 
	/* .Validate = */ NULL, 
	/* .Load     = */ iLoadDoomInternal, 
	/* .Save     = */ NULL, 
	/* .Exts     = */ iFormatExtsDOOM
};

ILformat iFormatDOOM_FLAT = { 
	/* .Validate = */ NULL, 
	/* .Load     = */ iLoadDoomFlatInternal, 
	/* .Save     = */ NULL, 
	/* .Exts     = */ iFormatExtsDOOM
};

#endif
