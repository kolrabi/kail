//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 2014-05-21 by Björn Paetzel
//
// Filename: src-IL/include/il_dcx.h
//
// Description: Reads from a .dcx file.
//
//-----------------------------------------------------------------------------


#ifndef DCX_H
#define DCX_H

#include "il_internal.h"


#pragma pack(push)
#pragma pack(1)

typedef struct DCXHEAD
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
} IL_PACKSTRUCT DCXHEAD;

#pragma pack(pop)

#endif//PCX_H
