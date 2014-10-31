//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/16/2009
//
// Filename: src-IL/src/il_texture.c
//
// Description: Reads from a Medieval II: Total War	(by Creative Assembly)
//				Texture (.texture) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_TEXTURE

ILboolean  iLoadDdsInternal(ILimage *image);
ILboolean  iIsValidDds(SIO *io);

static ILboolean iIsValidTexture(SIO *io) {
  // don't know of any magic value, so just seek forward and check for dds
  ILboolean   IsDDS;
  ILuint      Start = SIOtell(io);

  SIOseek(io, 48, IL_SEEK_CUR);
  IsDDS = iIsValidDds(io);
  SIOseek(io, Start, IL_SEEK_SET);

  return IsDDS;
}

//! Reads from a memory "lump" that contains a TEXTURE
static ILboolean iLoadTextureInternal(ILimage* image) {
  // From http://forums.totalwar.org/vb/showthread.php?t=70886, all that needs to be done
  //  is to strip out the first 48 bytes, and then it is DDS data.
  SIOseek(&image->io, 48, IL_SEEK_CUR);
  return iLoadDdsInternal(image);
}

static ILconst_string iFormatExtsTEXTURE[] = { 
  IL_TEXT("texture"), 
  NULL 
};

ILformat iFormatTEXTURE = { 
  /* .Validate = */ iIsValidTexture, 
  /* .Load     = */ iLoadTextureInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsTEXTURE
};

#endif//IL_NO_TEXTURE

