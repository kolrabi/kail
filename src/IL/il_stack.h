//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_stack.h
//
// Description: The main image stack
//
//-----------------------------------------------------------------------------

#ifndef IMAGESTACK_H
#define IMAGESTACK_H

#include "il_internal.h"


// Just a guess...seems large enough
#define I_STACK_INCREMENT 1024

typedef struct iFree
{
  ILuint  Name;
  void  *Next;
} iFree;


// Internal functions
ILboolean iEnlargeStack(void);
void      iFreeMem(void);
void      iBindImage(ILuint);
void      iInitIL(void);
void      iShutDownIL(void);
ILboolean iIsImage(ILuint Image);
ILuint    iCreateSubImage(ILimage *Image, ILenum Type, ILuint Num);
ILboolean iActiveFace(ILuint Number);
ILboolean iActiveImage(ILuint Number);
ILboolean iActiveLayer(ILuint Number);
ILboolean iActiveMipmap(ILuint Number);
void iDeleteImages(ILsizei Num, const ILuint *Images);
void iGenImages(ILsizei Num, ILuint *Images);
ILimage * iGetSelectedImage(const IL_IMAGE_SELECTION *CurSel);
#endif//IMAGESTACK_H
