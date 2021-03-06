//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/sgi.h
//
// Description: Reads from and writes to SGI graphics files.
//
//-----------------------------------------------------------------------------


#ifndef SGI_H
#define SGI_H

#include "il_internal.h"

#include "pack_push.h"
typedef struct iSgiHeader
{
	ILshort		MagicNum;	// IRIS image file magic number
	ILbyte		Storage;	// Storage format
	ILubyte		Bpc;			// Number of bytes per pixel channel
	ILushort	Dim;			// Number of dimensions
											//  1: single channel, 1 row with XSize pixels
											//  2: single channel, XSize*YSize pixels
											//  3: ZSize channels, XSize*YSize pixels
	
	ILushort	XSize;		// X size in pixels
	ILushort	YSize;		// Y size in pixels
	ILushort	ZSize;		// Number of channels
	ILint			PixMin;		// Minimum pixel value
	ILint			PixMax;		// Maximum pixel value
	ILint			Dummy1;		// Ignored
	ILbyte		Name[80];	// Image name
	ILint			ColMap;		// Colormap ID
	ILbyte		Dummy[404];	// Ignored
} iSgiHeader;
#include "pack_pop.h"

// Sgi format #define's
#define SGI_VERBATIM		0
#define SGI_RLE					1
#define SGI_MAGICNUM		474

// Sgi colormap types
#define SGI_COLMAP_NORMAL	0
#define SGI_COLMAP_DITHERED	1
#define SGI_COLMAP_SCREEN	2
#define SGI_COLMAP_COLMAP	3

#endif//SGI_H
