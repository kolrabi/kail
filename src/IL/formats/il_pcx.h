//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pcx.h
//
// Description: Reads and writes from/to a .pcx file.
//
//-----------------------------------------------------------------------------


#ifndef PCX_H
#define PCX_H

#include "il_internal.h"


#include "pack_push.h"

typedef struct PCXHEAD
{
	ILubyte		Manufacturer;
	ILubyte		Version;
	ILubyte		Encoding;
	ILubyte		Bpp;
	ILushort	Xmin, Ymin, Xmax, Ymax;
	ILushort	HDpi;
	ILushort	VDpi;
	ILubyte		ColMap[48];
	ILubyte		Reserved;
	ILubyte		NumPlanes;
	ILushort	Bps;
	ILushort	PaletteInfo;
	ILushort	HScreenSize;
	ILushort	VScreenSize;
	ILubyte		Filler[54];
} PCXHEAD;

#include "pack_pop.h"

// For checking and reading
ILboolean iIsValidPcx(SIO* io);
ILboolean iCheckPcx(PCXHEAD *Header);
ILboolean iUncompressPcx(ILimage* image, PCXHEAD *Header);
ILboolean iUncompressSmall(ILimage* image, PCXHEAD *Header);

#endif//PCX_H
