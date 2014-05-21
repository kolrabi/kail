//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C)      2014 by Björn Paetzel
// Last modified: 2014-05-21
//
//-----------------------------------------------------------------------------
#ifndef IL_FORMATS_H
#define IL_FORMATS_H

#include "il_internal.h"

typedef ILboolean (*ILformatLoadFunc)(ILimage *);
typedef ILboolean (*ILformatSaveFunc)(ILimage *);
typedef ILboolean (*ILformatValidateFunc)(SIO *);

typedef struct {
  ILformatValidateFunc  Validate;
  ILformatLoadFunc      Load;
  ILformatSaveFunc      Save;
  ILconst_string *      Exts;
} ILformat;

const ILformat *iGetFormat(ILenum id);
ILenum iIdentifyFormat(SIO *io);
ILenum iIdentifyFormatExt(ILconst_string ext);

void iInitFormats();
void iDeinitFormats();

#endif
