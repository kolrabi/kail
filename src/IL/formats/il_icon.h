//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_icon.h
//
// Description: Reads from a Windows icon (.ico) file.
//
//-----------------------------------------------------------------------------


#ifndef ICON_H
#define ICON_H

#include "il_internal.h"

#include "pack_push.h"

typedef struct ICODIR
{
	ILshort		Reserved;	// Reserved (must be 0)
	ILshort		Type;		// Type (1 for icons, 2 for cursors)
	ILshort		Count;		// How many different images?
} ICODIR;

typedef struct ICODIRENTRY
{
	ILubyte		Width;			// Width, in pixels
	ILubyte		Height;			// Height, in pixels
	ILubyte		NumColours;		// Number of colors in image (0 if >=8bpp)
	ILubyte		Reserved;		// Reserved (must be 0)
	ILshort		Planes;			// Colour planes
	ILshort		Bpp;			// Bits per pixel
	ILuint		SizeOfData;		// How many bytes in this resource?
	ILuint		Offset;			// Offset from beginning of the file
} ICODIRENTRY;

typedef struct INFOHEAD
{
	ILint		Size;
	ILint		Width;
	ILint		Height;
	ILshort		Planes;
	ILshort		BitCount;
	ILint		Compression;
	ILint		SizeImage;
	ILint		XPixPerMeter;
	ILint		YPixPerMeter;
	ILint		ColourUsed;
	ILint		ColourImportant;
} INFOHEAD;

typedef struct ICOIMAGE
{
	INFOHEAD	Head;
	ILubyte		*Pal;	// Palette
	ILubyte		*Data;	// XOR mask
	ILubyte		*AND;	// AND mask
} ICOIMAGE;

#include "pack_pop.h"

#endif//ICON_H
