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

#ifdef _WIN32
	#pragma pack(push, bmp_struct, 1)
#endif
typedef struct BMPHEAD {
	ILbyte		bfType[2];
	ILint		bfSize;
	ILuint		bfReserved;
	ILint		bfDataOff;
	ILint		biSize;
	ILint		biWidth;
	ILint		biHeight;
	ILshort		biPlanes;
	ILshort		biBitCount;
	ILint		biCompression;
	ILint		biSizeImage;
	ILint		biXPelsPerMeter;
	ILint		biYPelsPerMeter;
	ILint		biClrUsed;
	ILint		biClrImportant;
} IL_PACKSTRUCT BMPHEAD;

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
} IL_PACKSTRUCT OS2_HEAD;
#ifdef _WIN32
	#pragma pack(pop, bmp_struct)
#endif

#endif//BMP_H
