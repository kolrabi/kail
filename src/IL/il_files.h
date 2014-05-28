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

// Declaration of default read/write functions, used internally e.g. for ilLoadImage
__FILES_EXTERN ILHANDLE   ILAPIENTRY iDefaultOpenR(ILconst_string FileName);
__FILES_EXTERN ILHANDLE   ILAPIENTRY iDefaultOpenW(ILconst_string FileName);
__FILES_EXTERN void       ILAPIENTRY iDefaultClose(ILHANDLE Handle);
__FILES_EXTERN ILint      ILAPIENTRY iDefaultGetc(ILHANDLE Handle);
__FILES_EXTERN ILuint     ILAPIENTRY iDefaultRead(ILHANDLE Handle, void *Buffer, ILuint Size, ILuint Number);
__FILES_EXTERN ILint      ILAPIENTRY iDefaultSeek(ILHANDLE Handle, ILint64 Offset, ILuint Mode);
__FILES_EXTERN ILuint     ILAPIENTRY iDefaultTell(ILHANDLE Handle);
__FILES_EXTERN ILint      ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle);
__FILES_EXTERN ILint      ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle);

// Functions to set file or lump for reading/writing
__FILES_EXTERN void       iSetInputFile(ILimage *, ILHANDLE File);
__FILES_EXTERN void       iSetInputLump(ILimage *, const void *Lump, ILuint Size);
__FILES_EXTERN void       iSetOutputFile(ILimage *, ILHANDLE File);
__FILES_EXTERN void       iSetOutputLump(ILimage *, void *Lump, ILuint Size);
__FILES_EXTERN void       iSetOutputFake(ILimage *);

#endif//FILES_H
