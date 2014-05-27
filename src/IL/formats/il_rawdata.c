//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/rawdata.c
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
//#ifndef IL_NO_DATA
#include "il_manip.h"

static ILboolean iLoadDataInternal(ILimage *, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);

//! Reads a raw data file
ILboolean ILAPIENTRY ilLoadData(ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp) {
	ILHANDLE	RawFile;
	ILboolean	bRaw = IL_FALSE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->io.openReadOnly == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// No need to check for raw data
	/*if (!iCheckExtension(FileName, "raw")) {
		ilSetError(IL_INVALID_EXTENSION);
		return bRaw;
	}*/

	RawFile = iCurImage->io.openReadOnly(FileName);
	if (RawFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bRaw;
	}

	bRaw = ilLoadDataF(RawFile, Width, Height, Depth, Bpp);
	iCurImage->io.close(RawFile);

	return bRaw;
}


//! Reads an already-opened raw data file
ILboolean ILAPIENTRY ilLoadDataF(ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (File == NULL) {
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	iSetInputFile(iCurImage, File);
	FirstPos = iCurImage->io.tell(iCurImage->io.handle);
	bRet = iLoadDataInternal(iCurImage, Width, Height, Depth, Bpp);
	iCurImage->io.seek(iCurImage->io.handle, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a raw data memory "lump"
ILboolean ILAPIENTRY ilLoadDataL(void *Lump, ILuint Size, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	iSetInputLump(iCurImage, Lump, Size);
	return iLoadDataInternal(iCurImage, Width, Height, Depth, Bpp);
}


// Internal function to load a raw data image
ILboolean iLoadDataInternal(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	if ((Bpp != 1) && (Bpp != 3) && (Bpp != 4)) {
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	SIO *io = &Image->io;

	if (!ilTexImage_(Image, Width, Height, Depth, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	// Tries to read the correct amount of data
	if (SIOread(io, Image->Data, Width * Height * Depth * Bpp, 1) != 1)
		return IL_FALSE;

	if (Image->Bpp == 1)
		Image->Format = IL_LUMINANCE;
	else if (Image->Bpp == 3)
		Image->Format = IL_RGB;
	else  // 4
		Image->Format = IL_RGBA;

	return ilFixImage();
}


//! Save the current image to FileName as raw data
ILboolean ILAPIENTRY ilSaveData(ILconst_string FileName)
{
	ILHANDLE DataFile;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->io.openWrite == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	DataFile = iCurImage->io.openWrite(FileName);
	if (DataFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	SIO *io = &iCurImage->io;

	SIOwrite(io, iCurImage->Data, 1, iCurImage->SizeOfData);
	SIOclose(io);

	return IL_TRUE;
}

//#endif//IL_NO_DATA
