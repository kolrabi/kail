//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 01/23/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_il_psd.c
//
// Description: Reads from a PhotoShop (.psd) file.
//
//-----------------------------------------------------------------------------


#ifndef PSD_H
#define PSD_H

#include "il_internal.h"

#include "pack_push.h"

typedef struct PSDHEAD
{
	ILubyte		Signature[4];
	ILushort	Version;
	ILubyte		Reserved[6];
	ILushort	Channels;
	ILuint		Height;
	ILuint		Width;
	ILushort	Depth;
	ILushort	Mode;
} PSDHEAD;

#include "pack_pop.h"

ILushort	ChannelNum;

ILboolean	iCheckPsd(PSDHEAD *Header);
ILboolean	iLoadPsdInternal(ILimage *image);
ILboolean	ReadPsd(ILimage* image, PSDHEAD *Head);
ILboolean	ReadGrey(ILimage* image, PSDHEAD *Head);
ILboolean	ReadIndexed(ILimage* image, PSDHEAD *Head);
ILboolean	ReadRGB(ILimage* image, PSDHEAD *Head);
ILboolean	ReadCMYK(ILimage* image, PSDHEAD *Head);
ILuint		*GetCompChanLen(PSDHEAD *Head);
ILboolean	PsdGetData(ILimage* image, PSDHEAD *Head, void *Buffer, ILboolean Compressed);
ILboolean	ParseResources(ILimage* image, ILuint ResourceSize, ILubyte *Resources);
ILboolean	GetSingleChannel(ILimage* image, PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed);
ILboolean	iSavePsdInternal(ILimage* image);



#endif//PSD_H
