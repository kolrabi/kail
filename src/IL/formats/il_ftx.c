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

// Internal function used to load the FTX.
static ILboolean iLoadFtxInternal(ILimage *Image)
{
	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;

	FTX_HEAD Head;
	if (SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	UInt(&Head.Width);
	UInt(&Head.Height);
	UInt(&Head.HasAlpha); // Kind of backwards from what I would think...

	//@TODO: Right now, it appears that all images are in RGBA format.  See if I can find specs otherwise
	//  or images that load incorrectly like this.
	//if (HasAlpha == 0) {  // RGBA format
		if (!ilTexImage_(Image, Head.Width, Head.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	//}
	//else if (HasAlpha == 1) {  // RGB format
	//	if (!ilTexImage(Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
	//		return IL_FALSE;
	//}
	//else {  // Unknown format
	//	iSetError(IL_INVALID_FILE_HEADER);
	//	return IL_FALSE;
	//}

	// The origin will always be in the upper left.
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	// All we have to do for this format is read the raw, uncompressed data.
	if (SIOread(io, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData)
		return IL_FALSE;

	return IL_TRUE;
}

ILconst_string iFormatExtsFTX[] = { 
	IL_TEXT("ftx"), 
	NULL 
};

ILformat iFormatFTX = { 
	.Validate = NULL, 
	.Load     = iLoadFtxInternal, 
	.Save     = NULL, 
	.Exts     = iFormatExtsFTX
};

#endif//IL_NO_FTX

