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
ILboolean iGetBoolean(ILenum Mode);
ILint     iGetInteger(ILimage *Image, ILenum Mode);
ILboolean iIsEnabled(ILenum Mode);
void      iSetInteger(ILimage *Image, ILenum Mode, ILint Param);
void      iSetIntegerv(ILimage *Image, ILenum Mode, ILint *Param);
void      iSetString(ILimage *Image, ILenum Mode, const char *String_);
ILboolean iOriginFunc(ILenum Mode);
ILboolean iTypeFunc(ILenum Mode);

//
// Various states
//

typedef struct IL_STATES
{
  // Origin states
  ILboolean ilOriginSet;
  ILenum    ilOriginMode;

  // Format and type states
  ILboolean ilFormatSet;
  ILboolean ilTypeSet;
  ILenum    ilFormatMode;
  ILenum    ilTypeMode;

  // File mode states
  ILboolean ilOverWriteFiles;

  // Palette states
  ILboolean ilAutoConvPal;

  // Load fail states
  ILboolean ilDefaultOnFail;

  // Key colour states
  ILboolean   ilUseKeyColour;
  ILfloat     ilKeyColourRed;
  ILfloat     ilKeyColourGreen;
  ILfloat     ilKeyColourBlue;
  ILfloat     ilKeyColourAlpha;

  // Alpha blend states
  ILboolean ilBlitBlend;

  // Compression states
  ILenum    ilCompression;

  // Interlace states
  ILenum    ilInterlace;

  // Quantization states
  ILenum    ilQuantMode;
  ILuint    ilNeuSample;
  ILuint    ilQuantMaxIndexs;

  // DXTC states
  ILboolean ilKeepDxtcData;
  ILboolean ilUseNVidiaDXT;
  ILboolean ilUseSquishDXT;

  // clear colour
  ILfloat ClearColour[4];
  ILuint  ClearIndex;

  //
  // Format-specific states
  //

  ILboolean ilTgaCreateStamp;
  ILuint    ilJpgQuality;
  ILboolean ilPngInterlace;
  ILboolean ilTgaRle;
  ILboolean ilBmpRle;
  ILboolean ilSgiRle;
  ILenum    ilJpgFormat;
  ILboolean ilJpgProgressive;
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
