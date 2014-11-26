//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2001-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_manip.h
//
// Description: Image manipulation
//
//-----------------------------------------------------------------------------

#ifndef MANIP_H
#define MANIP_H

#ifdef _cplusplus
extern "C" {
#endif

ILboolean iDefaultImage(ILimage *Image);
ILboolean iClampNTSC(ILimage *Image);
ILboolean iSetAlpha(ILimage *Image, ILdouble AlphaValue);
ILubyte* 	iGetAlpha(ILimage *Image, ILenum Type);
void 			iSetPixels(ILimage *Image, ILint XOff, ILint YOff, ILint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);

#ifdef _cplusplus
}
#endif

#endif//MANIP_H
