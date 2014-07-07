//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/26/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_pxr.c
//
// Description: Reads from a Pxrar (.pxr) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PXR
#include "il_manip.h"
#include "il_endian.h"

#include "pack_push.h"
typedef struct PIXHEAD
{
  ILubyte   Signature[2];     // 0000 80 e8
  ILubyte   Reserved1[414];   // 0002 padding
  ILushort  Height;           // 01A0 height of image in pixels
  ILushort  Width;            // 01A2 width of image in pixels
  ILushort  Height2;          // 01A4 height again?
  ILushort  Width2;           // 01A6 width again?
  ILubyte   Bpp;              // 01A8 8 = grayscale, 14 = rgb, 15 = rgba
  ILuint    Offset;           // 01A9 pointing to Offset2
  ILushort  Unknown;          // 01AD = 4?
  ILubyte   Reserved2[1+80];  // 01AF probably some padding
  ILuint    Offset2;          // 0200 pointing to image data 0400
  ILuint    Size;             // 0204 size of image data in bytes

  //ILubyte   Reserved3[592];
} PIXHEAD;
#include "pack_pop.h"

static ILboolean iIsValidPxr(SIO *io) {
  ILuint  Start = SIOtell(io);
  PIXHEAD Head;
  ILuint  Read = SIOread(io, &Head, 1, sizeof(Head));
  SIOseek(io, Start, IL_SEEK_SET);

  return Read == sizeof(Head) && !memcmp(Head.Signature, "\x80\xe8", 2);
}

// Internal function used to load the Pxr.
static ILboolean iLoadPxrInternal(ILimage *Image) {
  PIXHEAD   Head;
  SIO *     io;

    if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &Image->io;

  if (SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head))
    return IL_FALSE;

  UShort(&Head.Height);
  UShort(&Head.Width);

  switch (Head.Bpp) {
    case 0x08:
      iTexImage(Image, Head.Width, Head.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
      break;
    case 0x0E:
      iTexImage(Image, Head.Width, Head.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
      break;
    case 0x0F:
      iTexImage(Image, Head.Width, Head.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
      break;
    default:
      iSetError(IL_INVALID_FILE_HEADER);
      return IL_FALSE;
  }

  // FIXME: should we use offsets from header?
  SIOseek(io, 1024 - sizeof(Head), IL_SEEK_CUR);
  if (SIOread(io, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData) 
    return IL_FALSE;

  Image->Origin = IL_ORIGIN_UPPER_LEFT;
  return IL_TRUE;
}

static ILboolean iSavePxrInternal(ILimage *Image) {
  PIXHEAD   Head;
  SIO *     io;
  ILimage * TempImage = Image;
  ILuint    Written;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &Image->io;

  if (Image->Type != IL_UNSIGNED_BYTE) {
    TempImage = iConvertImage(Image, IL_RGB, IL_UNSIGNED_BYTE);
  }

  imemclear(&Head, sizeof(Head));
  Head.Signature[0] = 0x80;
  Head.Signature[1] = 0xe8;
  Head.Height       = TempImage->Height;
  Head.Width        = TempImage->Width;
  Head.Height2      = TempImage->Height;
  Head.Width2       = TempImage->Width;
  Head.Reserved1[2] = 1;

  Head.Offset   = 512;
  Head.Unknown  = 4;
  Head.Offset2  = 1024;

  switch (TempImage->Format) {
    case IL_LUMINANCE:  Head.Bpp = 0x08; break;
    case IL_RGB:        Head.Bpp = 0x0E; break;
    case IL_RGBA:       Head.Bpp = 0x0F; break;
    default:
      TempImage = iConvertImage(Image, IL_RGB, IL_UNSIGNED_BYTE);
      Head.Bpp = 0x0E;
  }

  if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
    if (TempImage == Image) {
      TempImage = iCloneImage(Image);
    }
    iFlipBuffer((ILubyte *)TempImage->Data, 1, TempImage->Bps, TempImage->Height);
  }

  Head.Size = TempImage->Height * TempImage->Width * iGetBppFormat(TempImage->Format);

  UShort(&Head.Height);
  UShort(&Head.Width);
  UShort(&Head.Height2);
  UShort(&Head.Width2);
  UInt  (&Head.Offset);
  UShort(&Head.Unknown);
  UInt  (&Head.Offset2);
  UInt  (&Head.Size);

  Written = SIOwrite(io, &Head, 1, sizeof(Head));
  if (Written != sizeof(Head)) {
    iSetError(IL_FILE_WRITE_ERROR);
    return IL_FALSE;
  }
  SIOpad(io, 1024 - Written);

  SIOwrite(io, TempImage->Data, iGetBppFormat(TempImage->Format), Head.Width * Head.Height);

  if (TempImage != Image)
    iCloseImage(TempImage);

  return IL_TRUE;
}

ILconst_string iFormatExtsPXR[] = { 
  IL_TEXT("pxr"), 
  NULL 
};

ILformat iFormatPXR = { 
  /* .Validate = */ iIsValidPxr, 
  /* .Load     = */ iLoadPxrInternal, 
  /* .Save     = */ iSavePxrInternal, 
  /* .Exts     = */ iFormatExtsPXR
};

#endif//IL_NO_PXR
