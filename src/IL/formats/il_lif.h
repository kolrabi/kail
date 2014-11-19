//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_lif.c
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------


#ifndef LIF_H
#define LIF_H

#include "il_internal.h"

#include "pack_push.h"
typedef struct LIF_HEAD
{
    char  Id[8];      //"Willy 7"
    ILuint  Version;    // Version Number (260)
    ILuint  Flags;      // Usually 50 (Palette, TeamColor0, TeamColor1)
    ILuint  Width;
  ILuint  Height;
    ILuint  PaletteCRC;   // CRC of palettes for fast comparison.
    ILuint  ImageCRC;   // CRC of the image.
  ILuint  PalOffset;    // Offset to the palette (not used).
  ILuint  TeamEffect0;  // Team effect offset 0
  ILuint  TeamEffect1;  // Team effect offset 1
} LIF_HEAD;

enum {
  LIF_Flag_Palette    = 1<<1,
  LIF_Flag_Alpha      = 1<<3,
  LIF_Flag_TeamColor0 = 1<<4,
  LIF_Flag_TeamColor1 = 1<<5,
};

#include "pack_pop.h"

#endif//LIF_H
