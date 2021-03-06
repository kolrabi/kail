//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 09/01/2003 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_bmp.h
//
// Description: Reads and writes to a bitmap (.bmp) file.
//
//-----------------------------------------------------------------------------


#ifndef BMP_H
#define BMP_H

#include "il_internal.h"

#include "pack_push.h"

typedef struct BMPHEAD {
	ILbyte	bfType[2];
	ILuint	bfSize;
	ILuint	bfReserved;
	ILuint	bfDataOff;
	ILuint	biSize;
	ILint		biWidth;
	ILint		biHeight;
	ILushort	biPlanes;
	ILushort	biBitCount;
	ILuint		biCompression;
	ILuint		biSizeImage;
	ILuint		biXPelsPerMeter;
	ILuint		biYPelsPerMeter;
	ILuint		biClrUsed;
	ILuint		biClrImportant;
} BMPHEAD;

typedef struct OS2_HEAD
{
	// Bitmap file header.
	ILushort	bfType;
	ILuint		biSize;
	ILshort		xHotspot;
	ILshort		yHotspot;
	ILuint		DataOff;

	// Bitmap core header.
	ILuint		cbFix;
	//2003-09-01: changed cx, cy to ushort according to MSDN
	ILushort		cx;
	ILushort		cy;
	ILushort	cPlanes;
	ILushort	cBitCount;
} OS2_HEAD;

#include "pack_pop.h"

#endif//BMP_H
