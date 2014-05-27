//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_raw.c
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_RAW

// oddly enough, this "raw" format has a header
#include "pack_push.h"
typedef struct {
	ILuint 		Width, Height, Depth;
	ILubyte   Bpp, Bpc;
} RAW_HEAD;
#include "pack_pop.h"

// Internal function to load a raw image
static ILboolean iLoadRawInternal(ILimage *Image)
{
	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO * 			io = &Image->io;
	RAW_HEAD 		Head;

	if (!SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head))
		return IL_FALSE;

	UInt(&Head.Width);
	UInt(&Head.Height);
	UInt(&Head.Depth);

	if (!ilTexImage_(Image, Head.Width, Head.Height, Head.Depth, Head.Bpp, 0, ilGetTypeBpc(Head.Bpc), NULL)) {
		return IL_FALSE;
	}

	// Tries to read the correct amount of data
	if (SIOread(io, Image->Data, 1, Image->SizeOfData) < Image->SizeOfData)
		return IL_FALSE;

	if (ilIsEnabled(IL_ORIGIN_SET)) {
		Image->Origin = ilGetInteger(IL_ORIGIN_MODE);
	}	else {
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	if (Image->Bpp == 1)
		Image->Format = IL_LUMINANCE;
	else if (Image->Bpp == 3)
		Image->Format = IL_RGB;
	else  // 4
		Image->Format = IL_RGBA;

	return ilFixImage();
}

// Internal function used to load the raw data.
static ILboolean iSaveRawInternal(ILimage *Image)
{
	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;
	RAW_HEAD 		Head;

	Head.Width 	= Image->Width;
	Head.Height = Image->Height;
	Head.Depth 	= Image->Depth;
	Head.Bpp 		= Image->Bpp;
	Head.Bpc 		= Image->Bpc;

	UInt(&Head.Width);
	UInt(&Head.Height);
	UInt(&Head.Depth);

	return SIOwrite(io, &Head,       sizeof(Head),  		1)	== 1 
	    && SIOwrite(io, Image->Data, Image->SizeOfData, 1) 	== 1;

	return IL_TRUE;
}

ILconst_string iFormatExtsRAW[] = { 
	NULL 
};

ILformat iFormatRAW = { 
	.Validate = NULL, 
	.Load     = iLoadRawInternal, 
	.Save     = iSaveRawInternal, 
	.Exts     = iFormatExtsRAW
};

#endif//IL_NO_RAW
