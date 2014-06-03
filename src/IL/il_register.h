//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_register.h
//
// Description: Allows the caller to specify user-defined callback functions
//				 to open files DevIL does not support, to parse files
//				 differently, or anything else a person can think up.
//
//-----------------------------------------------------------------------------


#ifndef REGISTER_H
#define REGISTER_H

#include "il_internal.h"

typedef struct iFormatL
{
	ILstring	Ext;
	IL_LOADPROC	Load;
	struct iFormatL *Next;
} iFormatL;

typedef struct iFormatS
{
	ILstring	Ext;
	IL_SAVEPROC	Save;
	struct iFormatS *Next;
} iFormatS;

#define I_LOAD_FUNC 0
#define I_SAVE_FUNC 1

ILboolean iLoadRegistered(ILconst_string FileName);
ILboolean iSaveRegistered(ILconst_string FileName);

ILboolean iRegisterLoad(ILconst_string Ext, IL_LOADPROC Load);
ILboolean iRegisterSave(ILconst_string Ext, IL_SAVEPROC Save);
ILboolean iRemoveLoad(ILconst_string Ext);
ILboolean iRemoveSave(ILconst_string Ext);

void iRegisterFormat(ILimage *Image, ILenum Format);
ILboolean iRegisterMipNum(ILimage *Image, ILuint Num);
ILboolean iRegisterNumFaces(ILimage *Image, ILuint Num);
ILboolean iRegisterNumImages(ILimage *Image, ILuint Num);
void iRegisterOrigin(ILimage *Image, ILenum Origin);
void iRegisterType(ILimage *Image, ILenum Type);
void iRegisterPal(ILimage *Image, void *Pal, ILuint Size, ILenum Type);

#endif//REGISTER_H
