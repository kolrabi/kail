//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 11/07/2008
//
// Filename: src-IL/src/il_states.h
//
// Description: State machine
//
//-----------------------------------------------------------------------------


#ifndef STATES_H
#define STATES_H

#include "il_internal.h"

ILboolean iAble(ILenum Mode, ILboolean Flag);
ILboolean iFormatFunc(ILenum Mode);
ILboolean iIsEnabled(ILenum Mode);
void      iSetString(ILimage *Image, ILenum Mode, ILconst_string String_);
void      iSetStringMB(ILimage *Image, ILenum Mode, const char *String_);
ILboolean iOriginFunc(ILenum Mode);
ILboolean iTypeFunc(ILenum Mode);

//
// Various states
//

enum {
  IL_STATE_FLAG_ORIGIN_SET                    = (1U<<0),
  IL_STATE_FLAG_FORMAT_SET                    = (1U<<1),
  IL_STATE_FLAG_TYPE_SET                      = (1U<<2),

  IL_STATE_FLAG_OVERWRITE_FILES               = (1U<<3),
  IL_STATE_FLAG_BLIT_BLEND_ALPHA              = (1U<<4),
  IL_STATE_FLAG_AUTO_CONVERT_PALETTE          = (1U<<5),
  IL_STATE_FLAG_RETURN_DEFAULT_IMAGE_ON_FAIL  = (1U<<6),
  IL_STATE_FLAG_USE_KEY_COLOUR                = (1U<<7),

  IL_STATE_FLAG_KEEP_DXTC_DATA                = (1U<<8),
  IL_STATE_FLAG_USE_SQUISH_DXT                = (1U<<9),

  IL_STATE_FLAG_INTERLACE                     = (1U<<11),
  IL_STATE_FLAG_TGA_USE_RLE                   = (1U<<12),
  IL_STATE_FLAG_BMP_USE_RLE                   = (1U<<13),
  IL_STATE_FLAG_SGI_USE_RLE                   = (1U<<14),
  IL_STATE_FLAG_JPG_PROGRESSIVE               = (1U<<15),
};

typedef struct IL_STATES
{
  ILuint      ilStateFlags;

  // Origin states
  ILenum      ilOriginMode;

  // Format and type states
  ILenum      ilFormatMode;
  ILenum      ilTypeMode;

  // Key colour states
  ILfloat     ilKeyColourRed;
  ILfloat     ilKeyColourGreen;
  ILfloat     ilKeyColourBlue;
  ILfloat     ilKeyColourAlpha;

  // Compression states
  ILenum      ilCompression;

  // Quantization states
  ILenum      ilQuantMode;
  ILuint      ilNeuSample;
  ILuint      ilQuantMaxIndexs;

  // DXTC states

  //
  // Format-specific states
  //

  ILuint    ilJpgQuality;
  ILenum    ilJpgFormat;
  ILenum    ilDxtcFormat;
  ILenum    ilPcdPicNum;
  ILint     ilPngAlphaIndex;    // this index should be treated as an alpha key (most formats use this rather than having alpha in the palette), -1 for none
                                // currently only used when writing out .png files and should obviously be set to -1 most of the time
  ILenum    ilVtfCompression;


  //
  // Format-specific strings
  //

  ILchar*   ilCHeader;

  // image selection
  ILenum    ilImageSelectionMode;

  // clear colour
  ILfloat   ClearColour[4];
  ILuint    ClearIndex;
} IL_STATES;

typedef struct IL_HINTS
{
  // Memory vs. Speed trade-off
  ILenum    MemVsSpeedHint;
  // Compression hints
  ILenum    CompressHint;
} IL_HINTS;

void iKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
void iGetKeyColour(ILclampf *Red, ILclampf *Green, ILclampf *Blue, ILclampf *Alpha);
void iPushAttrib(ILuint Bits);
void iPopAttrib(void);

#endif//STATES_H
