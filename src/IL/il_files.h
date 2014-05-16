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
__FILES_EXTERN ILHANDLE			ILAPIENTRY iDefaultOpen(ILconst_string FileName);
__FILES_EXTERN void		        ILAPIENTRY iDefaultClose(ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultGetc(ILHANDLE Handle);
__FILES_EXTERN ILuint			ILAPIENTRY iDefaultRead(ILHANDLE Handle, void *Buffer, ILuint Size, ILuint Number);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultSeekR(ILHANDLE Handle, ILint64 Offset, ILint Mode);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultSeekW(ILHANDLE Handle, ILint Offset, ILint Mode);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultTellR(ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultTellW(ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle);

// Functions to set file or lump for reading/writing
__FILES_EXTERN void				iSetInputFile(ILHANDLE File);
__FILES_EXTERN void				iSetInputLump(const void *Lump, ILuint Size);
__FILES_EXTERN void				iSetOutputFile(ILHANDLE File);
__FILES_EXTERN void				iSetOutputLump(void *Lump, ILuint Size);
__FILES_EXTERN void				iSetOutputFake(void);
 
__FILES_EXTERN ILHANDLE			ILAPIENTRY iGetFile(void);
__FILES_EXTERN const ILubyte*	ILAPIENTRY iGetLump(void);

__FILES_EXTERN ILuint			ILAPIENTRY ilprintf(const char *, ...);
__FILES_EXTERN void				ipad(ILuint NumZeros);

#endif//FILES_H
