//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pal.h
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------


#ifndef IL_PAL_H
#define IL_PAL_H

#include "il_internal.h"

ILboolean iCopyPalette(ILpal *Dest, ILpal *Src);
ILAPI ILboolean ILAPIENTRY iConvertImagePal(ILimage *Image, ILenum DestFormat);
ILboolean iLoadPal(ILimage *Image, ILconst_string FileName);
ILboolean iApplyPal(ILimage *CurImage, ILconst_string FileName);

#endif//IL_PAL_H
