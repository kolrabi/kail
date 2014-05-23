//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/include/il_jpeg.h
//
// Description: Jpeg (.jpg) functions
//
//-----------------------------------------------------------------------------

#ifndef JPEG_H
#define JPEG_H

#include "il_internal.h"

ILboolean iLoadJpegInternal(ILimage *);

/*
ILboolean iCheckJpg(ILubyte Header[2]);
ILboolean iIsValidJpeg(SIO* io);

#ifndef IL_USE_IJL
	ILboolean iLoadJpegInternal(ILimage *);
	ILboolean iSaveJpegInternal(ILimage *);
#else
	ILboolean iLoadJpegInternal(ILconst_string FileName, ILvoid *Lump, ILuint Size);
	ILboolean iSaveJpegInternal(ILconst_string FileName, ILvoid *Lump, ILuint Size);
#endif

*/
#endif//JPEG_H
