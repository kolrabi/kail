//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 07/07/2006
//
// Filename: src-IL/src/il_fastconv.c
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"

ILboolean iFastConvert(ILimage *Image, ILenum DestFormat)
{
	void * VoidPtr = Image->Data;
	ILubyte		*BytePtr 	= Image->Data;
	ILushort	*ShortPtr = (ILushort*)VoidPtr;
	ILuint		*IntPtr 	= (ILuint*)VoidPtr;
	ILfloat		*FloatPtr = (ILfloat*)VoidPtr;
	ILdouble	*DblPtr 	= (ILdouble*)VoidPtr;
	ILuint		SizeOfData, i=0;
	ILubyte 	TempByte 	= 0;
	ILushort 	TempShort = 0;
	ILuint 		TempInt 	= 0;
	ILfloat 	TempFloat = 0;
	ILdouble 	TempDbl 	= 0;

	switch (DestFormat)
	{
		case IL_RGB:
		case IL_BGR:
			if (Image->Format != IL_RGB && Image->Format != IL_BGR)
				return IL_FALSE;

			switch (Image->Type)
			{
				case IL_BYTE:
				case IL_UNSIGNED_BYTE:
					SizeOfData = Image->SizeOfData / 3;
					for (i = 0; i < SizeOfData; i++) {
						TempByte = BytePtr[0];
						BytePtr[0] = BytePtr[2];
						BytePtr[2] = TempByte;
						BytePtr += 3;
					}
					return IL_TRUE;

				case IL_SHORT:
				case IL_UNSIGNED_SHORT:
					SizeOfData = Image->SizeOfData / 6;  // 3*2
					for (i = 0; i < SizeOfData; i++) {
						TempShort = ShortPtr[0];
						ShortPtr[0] = ShortPtr[2];
						ShortPtr[2] = TempShort;
						ShortPtr += 3;
					}
					return IL_TRUE;

				case IL_INT:
				case IL_UNSIGNED_INT:
					SizeOfData = Image->SizeOfData / 12;  // 3*4
					for (i = 0; i < SizeOfData; i++) {
						TempInt = IntPtr[0];
						IntPtr[0] = IntPtr[2];
						IntPtr[2] = TempInt;
						IntPtr += 3;
					}
					return IL_TRUE;
					
				case IL_FLOAT:
					SizeOfData = Image->SizeOfData / 12;  // 3*4
					for (i = 0; i < SizeOfData; i++) {
						TempFloat = FloatPtr[0];
						FloatPtr[0] = FloatPtr[2];
						FloatPtr[2] = TempFloat;
						FloatPtr += 3;
					}
					return IL_TRUE;

				case IL_DOUBLE:
					SizeOfData = Image->SizeOfData / 24;  // 3*8
					for (i = 0; i < SizeOfData; i++) {
						TempDbl = DblPtr[0];
						DblPtr[0] = DblPtr[2];
						DblPtr[2] = TempDbl;
						DblPtr += 3;
					}
					return IL_TRUE;
			}
			break;

		case IL_RGBA:
		case IL_BGRA:
			if (Image->Format != IL_RGBA && Image->Format != IL_BGRA)
				return IL_FALSE;

			switch (Image->Type)
			{
				case IL_BYTE:
				case IL_UNSIGNED_BYTE:
					SizeOfData = Image->SizeOfData / 4;
					for (i = 0; i < SizeOfData; i++) {
						TempByte = BytePtr[0];
						BytePtr[0] = BytePtr[2];
						BytePtr[2] = TempByte;
						BytePtr += 4;
					}
					return IL_TRUE;

				case IL_SHORT:
				case IL_UNSIGNED_SHORT:
					SizeOfData = Image->SizeOfData / 8;  // 4*2
					for (i = 0; i < SizeOfData; i++) {
						TempShort = ShortPtr[0];
						ShortPtr[0] = ShortPtr[2];
						ShortPtr[2] = TempShort;
						ShortPtr += 4;
					}
					return IL_TRUE;

				case IL_INT:
				case IL_UNSIGNED_INT:
					SizeOfData = Image->SizeOfData / 16;  // 4*4
					for (i = 0; i < SizeOfData; i++) {
						TempInt = IntPtr[0];
						IntPtr[0] = IntPtr[2];
						IntPtr[2] = TempInt;
						IntPtr += 4;
					}
					return IL_TRUE;

				case IL_FLOAT:
					SizeOfData = Image->SizeOfData / 16;  // 4*4
					for (i = 0; i < SizeOfData; i++) {
						TempFloat = FloatPtr[0];
						FloatPtr[0] = FloatPtr[2];
						FloatPtr[2] = TempFloat;
						FloatPtr += 4;
					}
					return IL_TRUE;

				case IL_DOUBLE:
					SizeOfData = Image->SizeOfData / 32;  // 4*8
					for (i = 0; i < SizeOfData; i++) {
						TempDbl = DblPtr[0];
						DblPtr[0] = DblPtr[2];
						DblPtr[2] = TempDbl;
						DblPtr += 4;
					}
					return IL_TRUE;
			}
	}


	return IL_FALSE;
}

