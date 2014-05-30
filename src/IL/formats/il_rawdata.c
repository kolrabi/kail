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
	ILimage *Image = iGetCurImage();

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->io.openReadOnly == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILHANDLE File = Image->io.openReadOnly(FileName);
	if (File == NULL) {
		iSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	iSetInputFile(Image, File);
	ILboolean bRet = iLoadDataInternal(Image, Width, Height, Depth, Bpp);

	SIOclose(&Image->io);
	Image->io.handle = NULL;

	return bRet;
}


//! Reads an already-opened raw data file
ILboolean ILAPIENTRY ilLoadDataF(ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	ILimage *Image = iGetCurImage();

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (File == NULL) {
		iSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	iSetInputFile(Image, File);
	FirstPos = SIOtell(&Image->io);
	bRet = iLoadDataInternal(Image, Width, Height, Depth, Bpp);
	SIOseek(&Image->io, FirstPos, IL_SEEK_SET); // BP: why rewind?

	return bRet;
}


//! Reads from a raw data memory "lump"
ILboolean ILAPIENTRY ilLoadDataL(void *Lump, ILuint Size, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	ILimage *Image = iGetCurImage();

	iSetInputLump(Image, Lump, Size);
	return iLoadDataInternal(Image, Width, Height, Depth, Bpp);
}


// Internal function to load a raw data image
ILboolean iLoadDataInternal(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	if ((Bpp != 1) && (Bpp != 3) && (Bpp != 4)) {
		iSetError(IL_INVALID_VALUE);
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

	return IL_TRUE;
}


//! Save the current image to FileName as raw data
ILboolean ILAPIENTRY ilSaveData(ILconst_string FileName)
{
	ILHANDLE DataFile;
	ILimage *Image = iGetCurImage();

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->io.openWrite == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	DataFile = Image->io.openWrite(FileName);
	if (DataFile == NULL) {
		iSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	SIO *io = &Image->io;

	SIOwrite(io, Image->Data, 1, Image->SizeOfData);
	SIOclose(io);

	return IL_TRUE;
}

//#endif//IL_NO_DATA
