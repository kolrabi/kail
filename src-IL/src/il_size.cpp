//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/30/2009
//
// Filename: src-IL/src/il_size.c
//
// Description: Determines the size of output files for lump writing.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"


ILuint iTargaSize(ILimage* image);


// Global variables
ILint64 CurPos;  // Fake "file" pointer.
ILint64 MaxPos;


//! Fake seek function
ILint ILAPIENTRY iSizeSeek(ILHANDLE h, ILint64 Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			CurPos = Offset;
			if (CurPos > MaxPos)
				MaxPos = CurPos;
			break;

		case IL_SEEK_CUR:
			CurPos = CurPos + Offset;
			break;

		case IL_SEEK_END:
			CurPos = MaxPos + Offset;  // Offset should be negative in this case.
			break;

		default:
			ilSetError(IL_INTERNAL_ERROR);  // Should never happen!
			return -1;  // Error code
	}

	if (CurPos > MaxPos)
		MaxPos = CurPos;

	return 0;  // Code for success
}

ILuint ILAPIENTRY iSizeTell(ILHANDLE h)
{
	return CurPos;
}

ILint ILAPIENTRY iSizePutc(ILubyte Char, ILHANDLE h)
{
	CurPos++;
	if (CurPos > MaxPos)
		MaxPos = CurPos;
	return Char;
}

ILint ILAPIENTRY iSizeWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h)
{
	CurPos += Size * Number;
	if (CurPos > MaxPos)
		MaxPos = CurPos;
	return Number;
}


// While it might be tempting to optimize this for uncompressed files, there are two reasons
// while this may not be a good idea:
// 1. With the current solution, changes in the file handler will be reflected immediately without additional 
//    effort in the size computation
// 2. The uncompressed file handlers are usually not a performance concern here, considering that no data
//    is actually written to a file

//! Returns the size of the memory buffer needed to save the current image into this Type.
//  A return value of 0 is an error.
ILAPI ILuint	ILAPIENTRY ilDetermineSize(ILenum Type)
{
	MaxPos = CurPos = 0;
	iSetOutputFake();  // Sets iputc, iwrite, etc. to functions above.
	ilSaveFuncs(Type);
	return MaxPos;
}
