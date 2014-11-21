//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_rle.h
//
// Description: Functions for run-length encoding
//
//-----------------------------------------------------------------------------

#ifndef RLE_H
#define RLE_H

#include "il_internal.h"

//
// Rle compression
//

#define   IL_TGACOMP 0x01
#define   IL_PCXCOMP 0x02
#define   IL_SGICOMP 0x03
#define   IL_BMPCOMP 0x04
#define   IL_CUTCOMP 0x05

#define   TGA_MAX_RUN 128
#define   SGI_MAX_RUN 127
#define   BMP_MAX_RUN 255
#define   CUT_MAX_RUN 127

ILboolean iRleCompressLine(const ILubyte *ScanLine, ILuint Width, ILubyte Bpp, ILubyte *Dest, ILuint *DestWidth, ILenum CompressMode);
ILuint    iRleCompress(const ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte *Dest, ILenum CompressMode, ILuint *ScanTable);

#endif//RLE_H
