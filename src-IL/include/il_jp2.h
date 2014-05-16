//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 08/24/2008
//
// Filename: src-IL/src/il_jp2.h
//
// Description: Jpeg-2000 (.jp2) functions
//
//-----------------------------------------------------------------------------

#ifndef JP2_H
#define JP2_H

#include "il_internal.h"

ILboolean		iLoadJp2Internal(ILimage* image, jas_stream_t *Stream);
ILboolean		iSaveJp2Internal(ILimage* image);
jas_stream_t	*iJp2ReadStream();

#endif//JP2_H
