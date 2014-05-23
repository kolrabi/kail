//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pix.c
//
// Description: Reads from an Alias | Wavefront .pix file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PIX
#include "il_manip.h"
#include "il_endian.h"

#include "pack_push.h"

typedef struct PIXHEAD
{
	ILushort	Width;
	ILushort	Height;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Bpp;
} PIXHEAD;
#include "pack_pop.h"

// Internal function used to check if the HEADER is a valid Pix header.
static ILboolean iCheckPix(PIXHEAD *Header) {
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	if (Header->Bpp != 24)
		return IL_FALSE;
	//if (Header->OffY != Header->Height)
	//	return IL_FALSE;

	return IL_TRUE;
}


// Internal function used to get the Pix header from the current file.
static ILuint iGetPixHead(SIO* io, PIXHEAD *Header)
{
	ILuint read = io->read(io->handle, Header, 1, sizeof(PIXHEAD));

	UShort(&Header->Width);
	UShort(&Header->Height);
	UShort(&Header->OffX);
	UShort(&Header->OffY);
	UShort(&Header->Bpp);

	return read;
}


// Internal function to get the header and check it.
static ILboolean iIsValidPix(SIO* io) {
	PIXHEAD	Head;
	ILuint read = iGetPixHead(io, &Head);
	io->seek(io->handle, -read, IL_SEEK_CUR);

	if (read == sizeof(Head))
		return iCheckPix(&Head);
	else
		return IL_FALSE;
}


// Internal function used to load the Pix.
static ILboolean iLoadPixInternal(ILimage* image) {
	PIXHEAD	Header;
	ILuint	i, j;
	ILubyte	ByteHead, Colour[3];

	if (image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPixHead(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckPix(&Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage_(image, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < image->SizeOfData; ) {
		ByteHead = image->io.getchar(image->io.handle);
		if (image->io.read(image->io.handle, Colour, 1, 3) != 3)
			return IL_FALSE;
		for (j = 0; j < ByteHead; j++) {
			image->Data[i++] = Colour[0];
			image->Data[i++] = Colour[1];
			image->Data[i++] = Colour[2];
		}
	}

	image->Origin = IL_ORIGIN_UPPER_LEFT;

	return ilFixImage();
}

ILconst_string iFormatExtsPIX[] = { 
  IL_TEXT("pix"), 
  NULL 
};

ILformat iFormatPIX = { 
  .Validate = iIsValidPix, 
  .Load     = iLoadPixInternal, 
  .Save     = NULL, 
  .Exts     = iFormatExtsPIX
};

#endif//IL_NO_PIX
