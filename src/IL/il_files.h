//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/include/il_files.h
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------

#ifndef FILES_H
#define FILES_H

#if defined (__FILES_C)
#define __FILES_EXTERN
#else
#define __FILES_EXTERN extern
#endif
#include <IL/il.h>

__FILES_EXTERN void       iSetOutputFake(ILimage *);


// "Fake" size functions, used to determine the size of write lumps
// Definitions are in il_size.cpp
void      ILAPIENTRY iSizeClose   (ILHANDLE h);
ILint     ILAPIENTRY iSizeSeek    (ILHANDLE h, ILint64 Offset, ILuint Mode);
ILuint    ILAPIENTRY iSizeTell    (ILHANDLE h);
ILint     ILAPIENTRY iSizePutc    (ILubyte Char, ILHANDLE h);
ILint     ILAPIENTRY iSizeGetc    (ILHANDLE h);
ILuint    ILAPIENTRY iSizeWrite   (const void *Buffer, ILuint Size, ILuint Number, ILHANDLE h);
ILuint    ILAPIENTRY iSizeRead    (ILHANDLE h, void *Buffer, ILuint Size, ILuint Number);

#endif//FILES_H
