//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/04/2009
//
// Filename: src-IL/src/il_utility.c
//
// Description: Utility functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"


/** Returns the channel count of any Format. */
ILAPI ILubyte ILAPIENTRY iGetBppFormat(ILenum Format)
{
	switch (Format)
	{
		case IL_COLOUR_INDEX:
		case IL_LUMINANCE:
		case IL_ALPHA:
			return 1;
		case IL_LUMINANCE_ALPHA:
			return 2;
		case IL_RGB:
		case IL_BGR:
			return 3;
		case IL_RGBA:
		case IL_BGRA:
			return 4;
	}
	return 0;
}


/** Returns the format of any channel count. */
ILAPI ILenum ILAPIENTRY iGetFormatBpp(ILubyte Bpp)
{
	switch (Bpp)
	{
		case 1:
			return IL_LUMINANCE;
		case 2:
			return IL_LUMINANCE_ALPHA;
		case 3:
			return IL_RGB;
		case 4:
			return IL_RGBA;
	}
	return 0;
}


// Returns the bpc of any Type
ILAPI ILubyte ILAPIENTRY iGetBpcType(ILenum Type)
{
	switch (Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			return 1;
		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
		case IL_HALF:
			return 2;
		case IL_INT:
		case IL_UNSIGNED_INT:
		case IL_FLOAT:
			return 4;
		case IL_DOUBLE:
			return 8;
	}
	return 0;
}


// Returns the type matching a bpc
ILAPI ILenum ILAPIENTRY iGetTypeBpc(ILubyte Bpc)
{
	switch (Bpc)
	{
		case 1:
			return IL_UNSIGNED_BYTE;
		case 2:
			return IL_UNSIGNED_SHORT;
		case 4:
			return IL_UNSIGNED_INT;
		case 8:
			return IL_DOUBLE;
	}
	return 0;
}


// Returns the bpp of any palette type (PalType)
ILAPI ILubyte ILAPIENTRY iGetBppPal(ILenum PalType)
{
	switch (PalType)
	{
		case IL_PAL_RGB24:
		case IL_PAL_BGR24:
			return 3;
		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			return 4;
	}
	return 0;
}

// Returns the base format of a palette type (PalType)
ILAPI ILenum ILAPIENTRY iGetPalBaseType(ILenum PalType)
{
	switch (PalType)
	{
		case IL_PAL_RGB24:
			return IL_RGB;
		case IL_PAL_RGB32:
			return IL_RGBA;  // Not sure
		case IL_PAL_RGBA32:
			return IL_RGBA;
		case IL_PAL_BGR24:
			return IL_BGR;
		case IL_PAL_BGR32:
			return IL_BGRA;  // Not sure
		case IL_PAL_BGRA32:
			return IL_BGRA;
	}

	return 0;
}


ILAPI ILuint ILAPIENTRY iGetMaskFormat(ILenum Format, ILubyte channel)
{
	switch (Format)
	{
		case IL_RGBA:
		case IL_RGB:	
			return (ILuint)(0x000000FF << (channel*8));

		case IL_BGRA:
		case IL_BGR:	
			if (channel == 3) {
				return 0xFF000000;
			} else {
				return 0x00FF0000 >> (channel*8UL);
			}
	}
	return 0;
}

ILAPI ILboolean ILAPIENTRY iFormatHasAlpha(ILenum Format)
{
	// TODO: paletted images
	return Format == IL_RGBA || Format == IL_BGRA || Format == IL_LUMINANCE_ALPHA || Format == IL_ALPHA;
}


// Returns the next power of 2 if Num isn't 2^n or returns Num if Num is 2^n
ILAPI ILuint ILAPIENTRY iNextPower2(ILuint n)
{	
	ILuint power = 1;
	while( power < n ) {
		power <<= 1;
	}
	return power;
}

ILAPI void ILAPIENTRY iMemSwap(ILubyte *s1, ILubyte *s2, const ILuint size)
{
	const ILuint block_size = 4096;
	const ILuint blocks = size/block_size;
	ILuint i;

	ILubyte *block = (ILubyte*)ialloc(block_size);
	if(block == NULL) return;
	for( i = 0; i < blocks; i++ ) {
		memcpy(block,s1,block_size);
		memcpy(s1,s2,block_size);
		memcpy(s2,block,block_size);
		s2 += block_size;
		s1 += block_size;
	}
	i = size - i*block_size;
	if( i > 0 ) {
		memcpy(block,s1,i);
		memcpy(s1,s2,i);
		memcpy(s2,block,i);
	}
	ifree(block);
	return;
}
