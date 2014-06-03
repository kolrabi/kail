//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/26/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_pxr.c
//
// Description: Reads from a Pxrar (.pxr) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PXR
#include "il_manip.h"
#include "il_endian.h"

#include "pack_push.h"
typedef struct PIXHEAD
{
	ILubyte  	Signature[2];
	ILubyte		Reserved1[413];
	ILushort	Height;
	ILushort	Width;
	ILubyte		Reserved2[4];
	ILubyte		Bpp;
	//ILubyte		Reserved3[598];
} PIXHEAD;
#include "pack_pop.h"

static ILboolean iIsValidPxr(SIO *io) {
	ILuint 	Start = SIOtell(io);
	PIXHEAD	Head;
	ILuint  Read = SIOread(io, &Head, 1, sizeof(Head));
	SIOseek(io, Start, IL_SEEK_SET);

	return Read == sizeof(Head) && !memcmp(Head.Signature, "\x80\xe8", 2);
}

// Internal function used to load the Pxr.
static ILboolean iLoadPxrInternal(ILimage *Image) {
	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	PIXHEAD		Head;
	SIO *     io = &Image->io;

	if (SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head))
		return IL_FALSE;

	UShort(&Head.Height);
	UShort(&Head.Width);

	switch (Head.Bpp) {
		case 0x08:
			iTexImage(Image, Head.Width, Head.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
			break;
		case 0x0E:
			iTexImage(Image, Head.Width, Head.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			break;
		case 0x0F:
			iTexImage(Image, Head.Width, Head.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			break;
		default:
			iSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	SIOseek(io, 1024 - sizeof(Head), IL_SEEK_CUR);
	if (SIOread(io, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData) 
		return IL_FALSE;

	Image->Origin = IL_ORIGIN_UPPER_LEFT;
	return IL_TRUE;
}

ILconst_string iFormatExtsPXR[] = { 
	IL_TEXT("pxr"), 
	NULL 
};

ILformat iFormatPXR = { 
	.Validate = iIsValidPxr, 
	.Load     = iLoadPxrInternal, 
	.Save     = NULL, 
	.Exts     = iFormatExtsPXR
};

#endif//IL_NO_PXR
