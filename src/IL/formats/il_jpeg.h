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

#ifndef IL_NO_JPG

#if defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable : 4005)  // Redefinitions in
  #pragma warning(disable : 4142)  //  jmorecfg.h
#endif

#include "jpeglib.h"

#if JPEG_LIB_VERSION < 62
  #warning DevIL was designed with libjpeg 6b or higher in mind.  Consider upgrading at www.ijg.org
#endif

ILboolean iLoadJpegInternal(ILimage *);

#endif

#endif//JPEG_H
