//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/15/2009
//
// Filename: src-IL/src/il_squish.cpp
//
// Description: Implements access to the Squish library.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"

#ifdef IL_USE_DXTC_SQUISH
#include <squish.h>

//! Compresses data to a DXT format using libsquish.
//  The data must be in unsigned byte RGBA format.  The alpha channel will be ignored if DxtType is IL_DXT1.
//  DxtSize is used to return the size in bytes of the DXTC data returned.
ILubyte* iSquishCompressDXTImpl(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize)
{
	ILuint	Size;  //@TODO: Support larger than 32-bit data?
	ILint	Flags;
	ILubyte	*DxtcData;

	if (Data == NULL) {  // We cannot operate on a null pointer.
		iSetError(IL_INVALID_PARAM);
		return NULL;
	}

	// The nVidia Texture Tools library does not support volume textures yet.
	if (Depth != 1) {
		iSetError(IL_INVALID_PARAM);
		return NULL;
	}

	switch (DxtFormat)
	{
		case IL_DXT1:  // Should these two be
		case IL_DXT1A: //  any different?
			Flags = squish::kDxt1;
			break;
		case IL_DXT3:
			Flags = squish::kDxt3;
			break;
		case IL_DXT5:
			Flags = squish::kDxt5;
			break;
		default:  // Does not support DXT2 or DXT4.
			iSetError(IL_INVALID_PARAM);
			break;
	}

	Size = squish::GetStorageRequirements(Width, Height, Flags);
	DxtcData = (ILubyte*)ialloc(Size);
	if (DxtcData == NULL)
		return NULL;

	squish::CompressImage(Data, Width, Height, DxtcData, Flags);

	*DxtSize = Size;
	return DxtcData;
}

extern "C" {
ILubyte* iSquishCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize) {
	return iSquishCompressDXTImpl(Data, Width, Height, Depth, DxtFormat, DxtSize);
}
}
#else
extern "C" {
// Let's have this so that the function is always created and exported, even if it does nothing.
ILubyte* iSquishCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize)
{
	(void)Data;
	(void)Width;
	(void)Height;
	(void)Depth;
	(void)DxtFormat;
	(void)DxtSize;
	
	iTrace("**** ilSquishCompressDXT not implemented!");
	iSetError(IL_INVALID_CONVERSION);
	return NULL;
}
}
#endif//IL_NO_DXTC_SQUISH
