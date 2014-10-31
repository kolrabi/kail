//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_targa.h
//
// Description: Targa (.tga) functions
//
//-----------------------------------------------------------------------------


#ifndef TARGA_H
#define TARGA_H

#include "il_internal.h"

#include "pack_push.h"
typedef struct TARGAHEAD
{
  ILubyte   IDLen;
  ILubyte   ColMapPresent;
  ILubyte   ImageType;
  ILshort   FirstEntry;
  ILushort  ColMapLen;
  ILubyte   ColMapEntSize;

  ILshort   OriginX;
  ILshort   OriginY;
  ILushort  Width;
  ILushort  Height;
  ILubyte   Bpp;
  ILubyte   ImageDesc;
} TARGAHEAD;

typedef struct TARGAFOOTER
{
  ILuint ExtOff;      // Extension Area Offset
  ILuint DevDirOff;   // Developer Directory Offset
  char   Signature[16]; // TRUEVISION-XFILE
  ILbyte Reserved;    // ASCII period '.'
  ILbyte NullChar;    // NULL
} TARGAFOOTER;

#define TGA_EXT_LEN   495
typedef struct TARGAEXT
{
  // Extension Area
  ILshort Size;       // should be TGA_EXT_LEN
  char    AuthName[41];   // the image author's name
  char    AuthComments[324];  // author's comments
  ILushort Month, Day, Year, Hour, Minute, Second; // internal date of file
  char    JobID[41];      // the job description (if any)
  ILushort JobHour, JobMin, JobSecs; // the job's time
  char    SoftwareID[41];   // the software that created this
  ILshort SoftwareVer;    // the software version number * 100
  ILbyte  SoftwareVerByte;  // the software version letter
  ILint KeyColor;     // the transparent colour
} TARGAEXT;

#include "pack_pop.h"

// Different Targa formats
#define TGA_NO_DATA       0
#define TGA_COLMAP_UNCOMP   1
#define TGA_UNMAP_UNCOMP    2
#define TGA_BW_UNCOMP     3
#define TGA_COLMAP_COMP     9
#define TGA_UNMAP_COMP      10
#define TGA_BW_COMP       11


// Targa origins
#define IMAGEDESC_ORIGIN_MASK 0x30
#define IMAGEDESC_TOPLEFT   0x20
#define IMAGEDESC_BOTLEFT   0x00
#define IMAGEDESC_BOTRIGHT    0x10
#define IMAGEDESC_TOPRIGHT    0x30


#endif//TARGA_H
