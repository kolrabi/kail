//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/12/2009
//
// Filename: src-IL/src/il_ftx.c
//
// Description: Reads from a Heavy Metal: FAKK2 (.ftx) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_FTX

#include "pack_push.h"
typedef struct {
	ILuint Width, Height, HasAlpha;
} FTX_HEAD;
#include "pack_pop.h"

static ILboolean iIsValidFtx(SIO *io) {
	FTX_HEAD Head;
	ILuint Pos = SIOtell(io);
	ILuint Read = SIOread(io, &Head, 1, sizeof(Head));
	ILuint Size, MinSize;

	SIOseek(io, 0, IL_SEEK_END);
	Size = SIOtell(io) - Pos;
	SIOseek(io, Pos, IL_SEEK_SET);

	if (Read != sizeof(Head))
		return IL_FALSE;

	UInt(&Head.Width);
	UInt(&Head.Height);
	UInt(&Head.HasAlpha); // Kind of backwards from what I would think...

	if (Head.HasAlpha != 0 && Head.HasAlpha != 1)
		return IL_FALSE;

	MinSize = Head.Width * Head.Height * (4-Head.HasAlpha) + sizeof(Head);
	if (Size < MinSize || MinSize == 0)
		return IL_FALSE;

	// not really conclusive
	return IL_TRUE;
}

// Internal function used to load the FTX.
static ILboolean iLoadFtxInternal(ILimage *Image)
{
	FTX_HEAD Head;
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	io = &Image->io;

	if (SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	UInt(&Head.Width);
	UInt(&Head.Height);
	UInt(&Head.HasAlpha); // Kind of backwards from what I would think...

	if (Head.HasAlpha == 0) {  // RGBA format
		if (!iTexImage(Image, Head.Width, Head.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	}	else if (Head.HasAlpha == 1) {  // RGB format
		if (!iTexImage(Image, Head.Width, Head.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	} else {  // Unknown format
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// The origin will always be in the upper left.
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	// All we have to do for this format is read the raw, uncompressed data.
	if (SIOread(io, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData)
		return IL_FALSE;

	return IL_TRUE;
}

static ILboolean iSaveFtxInternal(ILimage *Image)
{
	ILubyte *Data;
	ILuint i;
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	io = &Image->io;

	SaveLittleUInt(io, Image->Width);
	SaveLittleUInt(io, Image->Height);
	SaveLittleUInt(io, 0);

	Data = iConvertBuffer(Image->SizeOfData, Image->Format, IL_RGBA, Image->Type, IL_UNSIGNED_BYTE, NULL, Image->Data);
	if (!Data) {
		return IL_FALSE;
	}

	if (Image->Origin == IL_ORIGIN_UPPER_LEFT) {
		SIOwrite(io, Data, Image->Width*Image->Height*4, 1);
	} else {
		for (i=0; i < Image->Height; i++) {
			SIOwrite(io, Data + (Image->Height - i - 1)*4*Image->Width, 4*Image->Width, 1);
		}
	}	

	ifree(Data);
	return IL_TRUE;
}

static ILconst_string iFormatExtsFTX[] = { 
	IL_TEXT("ftx"), 
	NULL 
};

ILformat iFormatFTX = { 
	/* .Validate = */ iIsValidFtx, 
	/* .Load     = */ iLoadFtxInternal, 
	/* .Save     = */ iSaveFtxInternal, 
	/* .Exts     = */ iFormatExtsFTX
};

#endif//IL_NO_FTX

