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

ILboolean PcxGetHeader(SIO* io, PCXHEAD *header);
ILboolean PcxCheckHeader(PCXHEAD *Header);
ILimage * PcxDecode(SIO *io, PCXHEAD *Header);
ILboolean PcxEncode(SIO *io, ILimage *Image);

#endif//PCX_H
