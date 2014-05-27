//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_pal.c
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_pal.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

//! Loads a palette from FileName into the current image's palette.
ILboolean ILAPIENTRY ilLoadPal(ILconst_string FileName)
{
	ILenum type = ilTypeFromExt(FileName);

	// TODO: general check if type is a palette
	if ( type == IL_JASC_PAL
		|| type == IL_HALO_PAL
		|| type == IL_ACT_PAL
		|| type == IL_COL_PAL
		|| type == IL_PLT_PAL) {
		return ilSave(type, FileName);
	}

	ilSetError(IL_INVALID_EXTENSION);
	return IL_FALSE;
/*
	FILE		*f;
	ILboolean	IsPsp;
	char		Head[8];

	if (FileName == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCheckExtension(FileName, IL_TEXT("col"))) {
		return ilLoadColPal(FileName);
	}
	if (iCheckExtension(FileName, IL_TEXT("act"))) {
		return ilLoadActPal(FileName);
	}
	if (iCheckExtension(FileName, IL_TEXT("plt"))) {
		return ilLoadPltPal(FileName);
	}

#ifndef _UNICODE
	f = fopen(FileName, "rt");
#else
	f = _wfopen(FileName, L"rt");
#endif//_UNICODE
	if (f == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	fread(Head, 1, 8, f);
	if (!strncmp(Head, "JASC-PAL", 8))
		IsPsp = IL_TRUE;
	else
		IsPsp = IL_FALSE;

	fclose(f);

	if (IsPsp)
		return ilLoadJascPal(FileName);
	return ilLoadHaloPal(FileName);*/
}


ILboolean ILAPIENTRY ilSavePal(ILconst_string FileName)
{
	ILenum type = ilTypeFromExt(FileName);

	// TODO: general check if type is a palette
	if (type == IL_JASC_PAL) {
		return ilSave(type, FileName);
	}

	ilSetError(IL_INVALID_EXTENSION);
	return IL_FALSE;
	/*
	// ILstring Ext = iGetExtension(FileName);

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (FileName == NULL || iStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (!iCurImage->Pal.Palette || !iCurImage->Pal.PalSize || iCurImage->Pal.PalType == IL_PAL_NONE) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->io.openWrite == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	// FIXME: select different formats?
	
	ILboolean	bRet = IL_FALSE;
	iCurImage->io.handle = iCurImage->io.openWrite(FileName);
	if (iCurImage->io.handle != NULL) {
		bRet = iSaveJascPal(iCurImage);
		iCurImage->io.close(iCurImage->io.handle);
		iCurImage->io.handle = NULL;
		return bRet;
	} else {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}
	*/
}

// Assumes that Dest has nothing in it.
ILboolean iCopyPalette(ILpal *Dest, ILpal *Src)
{
	if (Src->Palette == NULL || Src->PalSize == 0)
		return IL_FALSE;

	Dest->Palette = (ILubyte*)ialloc(Src->PalSize);
	if (Dest->Palette == NULL)
		return IL_FALSE;

	memcpy(Dest->Palette, Src->Palette, Src->PalSize);

	Dest->PalSize = Src->PalSize;
	Dest->PalType = Src->PalType;

	return IL_TRUE;
}


ILAPI ILpal* ILAPIENTRY iCopyPal()
{
	ILpal *Pal;

	if (iCurImage == NULL || iCurImage->Pal.Palette == NULL ||
		iCurImage->Pal.PalSize == 0 || iCurImage->Pal.PalType == IL_PAL_NONE) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	Pal = (ILpal*)ialloc(sizeof(ILpal));
	if (Pal == NULL) {
		return NULL;
	}
	if (!iCopyPalette(Pal, &iCurImage->Pal)) {
		ifree(Pal);
		return NULL;
	}

	return Pal;
}


// Converts the palette to the DestFormat format.
ILAPI ILpal* ILAPIENTRY iConvertPal(ILpal *Pal, ILenum DestFormat)
{
	ILpal	*NewPal = NULL;
	ILuint	i, j, NewPalSize;

	// Checks to see if the current image is valid and has a palette
	if (Pal == NULL || Pal->PalSize == 0 || Pal->Palette == NULL || Pal->PalType == IL_PAL_NONE) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	/*if (Pal->PalType == DestFormat) {
		return NULL;
	}*/

	NewPal = (ILpal*)ialloc(sizeof(ILpal));
	if (NewPal == NULL) {
		return NULL;
	}
	NewPal->PalSize = Pal->PalSize;
	NewPal->PalType = Pal->PalType;

	switch (DestFormat)
	{
		case IL_PAL_RGB24:
		case IL_PAL_BGR24:
			switch (Pal->PalType)
			{
				case IL_PAL_RGB24:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_BGR24) {
						j = ilGetBppPal(Pal->PalType);
						for (i = 0; i < Pal->PalSize; i += j) {
							NewPal->Palette[i] = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGR24:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						j = ilGetBppPal(Pal->PalType);
						for (i = 0; i < Pal->PalSize; i += j) {
							NewPal->Palette[i] = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGR32:
				case IL_PAL_BGRA32:
					NewPalSize = (ILuint)((ILfloat)Pal->PalSize * 3.0f / 4.0f);
					NewPal->Palette = (ILubyte*)ialloc(NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i+2];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i];
						}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGB32:
				case IL_PAL_RGBA32:
					NewPalSize = (ILuint)((ILfloat)Pal->PalSize * 3.0f / 4.0f);
					NewPal->Palette = (ILubyte*)ialloc(NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
						}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i+2];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i];
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				default:
					ilSetError(IL_INVALID_PARAM);
					return NULL;
			}
			break;

		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			switch (Pal->PalType)
			{
				case IL_PAL_RGB24:
				case IL_PAL_BGR24:
					NewPalSize = Pal->PalSize * 4 / 3;
					NewPal->Palette = (ILubyte*)ialloc(NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if ((Pal->PalType == IL_PAL_BGR24 && (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32)) ||
						(Pal->PalType == IL_PAL_RGB24 && (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32))) {
							for (i = 0, j = 0; i < Pal->PalSize; i += 3, j += 4) {
								NewPal->Palette[j]   = Pal->Palette[i+2];
								NewPal->Palette[j+1] = Pal->Palette[i+1];
								NewPal->Palette[j+2] = Pal->Palette[i];
								NewPal->Palette[j+3] = 255;
							}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 3, j += 4) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
							NewPal->Palette[j+3] = 255;
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGB32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;

					if (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = 255;
						}
					}
					else {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i+2];
							NewPal->Palette[i+3] = 255;
						}
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGBA32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = Pal->Palette[i+3];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;
				
				case IL_PAL_BGR32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = 255;
						}
					}
					else {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i+2];
							NewPal->Palette[i+3] = 255;
						}
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGRA32:
					NewPal->Palette = (ILubyte*)ialloc(Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = Pal->Palette[i+3];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				default:
					ilSetError(IL_INVALID_PARAM);
					return NULL;
			}
			break;


		default:
			ilSetError(IL_INVALID_PARAM);
			return NULL;
	}

	NewPal->PalType = DestFormat;

	return NewPal;

alloc_error:
	ifree(NewPal);
	return NULL;
}


//! Converts the current image to the DestFormat format.
ILboolean ILAPIENTRY ilConvertPal(ILenum DestFormat)
{
	ILpal *Pal;

	if (iCurImage == NULL || iCurImage->Pal.Palette == NULL ||
		iCurImage->Pal.PalSize == 0 || iCurImage->Pal.PalType == IL_PAL_NONE) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Pal = iConvertPal(&iCurImage->Pal, DestFormat);
	if (Pal == NULL)
		return IL_FALSE;

	ifree(iCurImage->Pal.Palette);
	iCurImage->Pal.PalSize = Pal->PalSize;
	iCurImage->Pal.PalType = Pal->PalType;

	iCurImage->Pal.Palette = (ILubyte*)ialloc(Pal->PalSize);
	if (iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	memcpy(iCurImage->Pal.Palette, Pal->Palette, Pal->PalSize);

	ifree(Pal->Palette);
	ifree(Pal);
	
	return IL_TRUE;
}


// Sets the current palette.
ILAPI void ILAPIENTRY ilSetPal(ILpal *Pal)
{
	if (iCurImage->Pal.Palette && iCurImage->Pal.PalSize && iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(iCurImage->Pal.Palette);
	}

	if (Pal->Palette && Pal->PalSize && Pal->PalType != IL_PAL_NONE) {
		iCurImage->Pal.Palette = (ILubyte*)ialloc(Pal->PalSize);
		if (iCurImage->Pal.Palette == NULL)
			return;
		memcpy(iCurImage->Pal.Palette, Pal->Palette, Pal->PalSize);
		iCurImage->Pal.PalSize = Pal->PalSize;
		iCurImage->Pal.PalType = Pal->PalType;
	}
	else {
		iCurImage->Pal.Palette = NULL;
		iCurImage->Pal.PalSize = 0;
		iCurImage->Pal.PalType = IL_PAL_NONE;
	}

	return;
}

// Global variable
ILuint CurSort = 0;

typedef struct COL_CUBE
{
	ILubyte	Min[3];
	ILubyte	Val[3];
	ILubyte	Max[3];
} COL_CUBE;

int sort_func(void *e1, void *e2)
{
	return ((COL_CUBE*)e1)->Val[CurSort] - ((COL_CUBE*)e2)->Val[CurSort];
}


ILboolean ILAPIENTRY ilApplyPal(ILconst_string FileName)
{
	ILimage		Image, *CurImage = iCurImage;
	ILubyte		*NewData;
	ILuint		*PalInfo, NumColours, NumPix, MaxDist, DistEntry=0, i, j;
	ILint		Dist1, Dist2, Dist3;
	ILboolean	Same;
	ILenum		Origin;
//	COL_CUBE	*Cubes;

    if( iCurImage == NULL || (iCurImage->Format != IL_BYTE || iCurImage->Format != IL_UNSIGNED_BYTE) ) {
    	ilSetError(IL_ILLEGAL_OPERATION);
        return IL_FALSE;
    }

	NewData = (ILubyte*)ialloc(iCurImage->Width * iCurImage->Height * iCurImage->Depth);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	iCurImage = &Image;
	imemclear(&Image, sizeof(ILimage));
	// IL_PAL_RGB24, because we don't want to make parts transparent that shouldn't be.
	if (!ilLoadPal(FileName) || !ilConvertPal(IL_PAL_RGB24)) {
		ifree(NewData);
		iCurImage = CurImage;
		return IL_FALSE;
	}

	NumColours = Image.Pal.PalSize / 3;  // RGB24 is 3 bytes per entry.
	PalInfo = (ILuint*)ialloc(NumColours * sizeof(ILuint));
	if (PalInfo == NULL) {
		ifree(NewData);
		iCurImage = CurImage;
		return IL_FALSE;
	}

	NumPix = CurImage->SizeOfData / ilGetBppFormat(CurImage->Format);
	switch (CurImage->Format)
	{
		case IL_COLOUR_INDEX:
			iCurImage = CurImage;
			if (!ilConvertPal(IL_PAL_RGB24)) {
				ifree(NewData);
				ifree(PalInfo);
				return IL_FALSE;
			}

			NumPix = iCurImage->Pal.PalSize / ilGetBppPal(iCurImage->Pal.PalType);
			for (i = 0; i < NumPix; i++) {
				for (j = 0; j < Image.Pal.PalSize; j += 3) {
					// No need to perform a sqrt.
					Dist1 = (ILint)iCurImage->Pal.Palette[i] - (ILint)Image.Pal.Palette[j];
					Dist2 = (ILint)iCurImage->Pal.Palette[i+1] - (ILint)Image.Pal.Palette[j+1];
					Dist3 = (ILint)iCurImage->Pal.Palette[i+2] - (ILint)Image.Pal.Palette[j+2];
					PalInfo[j / 3] = Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3;
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				iCurImage->Pal.Palette[i] = DistEntry;
			}

			for (i = 0; i < iCurImage->SizeOfData; i++) {
				NewData[i] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			}
			break;
		case IL_RGB:
		case IL_RGBA:
			/*Cube = (COL_CUBE*)ialloc(NumColours * sizeof(COL_CUBE));
			// @TODO:  Check if ialloc failed here!
			for (i = 0; i < NumColours; i++)
				memcpy(&Cubes[i].Val, Image.Pal.Palette[i * 3], 3);
			for (j = 0; j < 3; j++) {
				qsort(Cubes, NumColours, sizeof(COL_CUBE), sort_func);
				Cubes[0].Min = 0;
				Cubes[NumColours-1] = UCHAR_MAX;
				NumColours--;
				for (i = 1; i < NumColours; i++) {
					Cubes[i].Min[CurSort] = Cubes[i-1].Val[CurSort] + 1;
					Cubes[i-1].Max[CurSort] = Cubes[i].Val[CurSort] - 1;
				}
				CurSort++;
				NumColours++;
			}*/
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp) {
				Same = IL_TRUE;
				if (i != 0) {
					for (j = 0; j < CurImage->Bpp; j++) {
						if (CurImage->Data[i-CurImage->Bpp+j] != CurImage->Data[i+j]) {
							Same = IL_FALSE;
							break;
						}
					}
				}
				if (Same) {
					NewData[i / CurImage->Bpp] = DistEntry;
					continue;
				}
				for (j = 0; j < Image.Pal.PalSize; j += 3) {
					// No need to perform a sqrt.
					Dist1 = (ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j];
					Dist2 = (ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j+1];
					Dist3 = (ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j+2];
					PalInfo[j / 3] = Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3;
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i / CurImage->Bpp] = DistEntry;
			}

			break;

		case IL_BGR:
		case IL_BGRA:
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp) {
				for (j = 0; j < NumColours; j++) {
					// No need to perform a sqrt.
					PalInfo[j] = ((ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j * 3]) *
						((ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j * 3]) + 
						((ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j * 3 + 1]) *
						((ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j * 3 + 1]) +
						((ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j * 3 + 2]) *
						((ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j * 3 + 2]);
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i / CurImage->Bpp] = DistEntry;
			}

			break;

		case IL_LUMINANCE:
		case IL_LUMINANCE_ALPHA:
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp ) {
				for (j = 0; j < NumColours; j++) {
					// No need to perform a sqrt.
					PalInfo[j] = ((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3]) + 
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 1]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 1]) +
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 2]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 2]);
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i] = DistEntry;
			}

			break;

		default:  // Should be no other!
			ilSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	iCurImage = CurImage;
	Origin = iCurImage->Origin;
	if (!ilTexImage_(iCurImage, iCurImage->Width, iCurImage->Height, iCurImage->Depth, 1,
		IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NewData)) {
		ifree(Image.Pal.Palette);
		ifree(PalInfo);
		ifree(NewData);
		return IL_FALSE;
	}
	iCurImage->Origin = Origin;

	iCurImage->Pal.Palette = Image.Pal.Palette;
	iCurImage->Pal.PalSize = Image.Pal.PalSize;
	iCurImage->Pal.PalType = Image.Pal.PalType;
	ifree(PalInfo);
	ifree(NewData);

	return IL_TRUE;
}
