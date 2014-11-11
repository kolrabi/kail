//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 2014 by Björn Ganster
//
// Filename: src-IL/src/il_bmp.c
//
// Description: Reads from and writes to a bitmap (.bmp) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_BMP

#include "il_bmp.h"
#include "il_endian.h"
#include "algo/il_rle.h"

#include <stdio.h>

#ifndef _WIN32
  #ifdef HAVE_STDINT_H
    #include <stdint.h>
    typedef uint32_t DWORD, *LPDWORD;
    typedef uint16_t WORD, *LPWORD;
    typedef uint8_t  BYTE, *LPBYTE;
  #else 
    // let's just hope for the best
    #warning "TODO: add correct definitions for your platform"
    typedef ILuint   DWORD, *LPDWORD;
    typedef ILushort WORD, *LPWORD;
    typedef ILuchar  BYTE, *LPBYTE;
  #endif
#endif

static void GetShiftFromMask(const ILuint Mask, ILuint * CONST_RESTRICT ShiftLeft, ILuint * CONST_RESTRICT ShiftRight);

// Internal function used to get the .bmp header from the current file.
static ILboolean iGetBmpHead(SIO* io, BMPHEAD * const Header)
{
  ILuint Pos = SIOtell(io);
  ILuint read = SIOread(io, Header, 1, sizeof(BMPHEAD));
  SIOseek(io, Pos, IL_SEEK_SET);

  Int  (&Header->bfSize);
  UInt (&Header->bfReserved);
  Int  (&Header->bfDataOff);
  Int  (&Header->biSize);
  Int  (&Header->biWidth);
  Int  (&Header->biHeight);
  Short(&Header->biPlanes);
  Short(&Header->biBitCount);
  Int  (&Header->biCompression);
  Int  (&Header->biSizeImage);
  Int  (&Header->biXPelsPerMeter);
  Int  (&Header->biYPelsPerMeter);
  Int  (&Header->biClrUsed);
  Int  (&Header->biClrImportant); 

  if (read == sizeof(BMPHEAD))
    return IL_TRUE;
  else
    return IL_FALSE;
}


static ILboolean iGetOS2Head(SIO* io, OS2_HEAD * const Header)
{
  ILuint Pos = SIOtell(io);
  ILuint read = SIOread(io, Header, 1, sizeof(OS2_HEAD));
  SIOseek(io, Pos, IL_SEEK_SET);

  if (read != sizeof(OS2_HEAD))
    return IL_FALSE;

  UShort(&Header->bfType);
  UInt  (&Header->biSize);
  Short (&Header->xHotspot);
  Short (&Header->yHotspot);
  UInt  (&Header->DataOff);
  UInt  (&Header->cbFix);

  //2003-09-01 changed to UShort according to MSDN
  UShort(&Header->cx);
  UShort(&Header->cy);
  UShort(&Header->cPlanes);
  UShort(&Header->cBitCount);

  return IL_TRUE;
}


// Internal function used to check if the HEADER is a valid .bmp header.
static ILboolean iCheckBmp (const BMPHEAD * CONST_RESTRICT Header)
{
  //if ((Header->bfType != ('B'|('M'<<8))) || ((Header->biSize != 0x28) && (Header->biSize != 0x0C)))
  if (Header->bfType[0] != 'B' || Header->bfType[1] != 'M')
    return IL_FALSE;
  if (Header->biSize < 0x28)
    return IL_FALSE;
  if (Header->biHeight == 0 || Header->biWidth < 1)
    return IL_FALSE;
  if (Header->biPlanes > 1)  // Not sure...
    return IL_FALSE;
  if(Header->biCompression != 0 && Header->biCompression != 1 &&
    Header->biCompression != 2 && Header->biCompression != 3)
    return IL_FALSE;
  if(Header->biCompression == 3 && Header->biBitCount != 16 && Header->biBitCount != 32)
    return IL_FALSE;
  if (Header->biBitCount != 1 && Header->biBitCount != 4 && Header->biBitCount != 8 &&
    Header->biBitCount != 16 && Header->biBitCount != 24 && Header->biBitCount != 32)
    return IL_FALSE;
  return IL_TRUE;
}


static ILboolean iCheckOS2 (const OS2_HEAD * CONST_RESTRICT Header)
{
  if ((Header->bfType != ('B'|('M'<<8))) || (Header->DataOff < 26) || (Header->cbFix < 12))
    return IL_FALSE;
  if (Header->cPlanes != 1)
    return IL_FALSE;
  if (Header->cx == 0 || Header->cy == 0)
    return IL_FALSE;
  if (Header->cBitCount != 1 && Header->cBitCount != 4 && Header->cBitCount != 8 &&
     Header->cBitCount != 24)
     return IL_FALSE;

  return IL_TRUE;
}


// Internal function to get the header and check it.
static ILboolean iIsValidBmp(SIO* io)
{
  char Sig[2];
  ILuint Pos = SIOtell(io);
  ILuint Read = SIOread(io, Sig, 1, 2);
  SIOseek(io, Pos, IL_SEEK_SET);
  return Read == 2 && memcmp(Sig, "BM", 2) == 0;
}


INLINE void GetShiftFromMask(const ILuint Mask, ILuint * CONST_RESTRICT ShiftLeft, ILuint * CONST_RESTRICT ShiftRight) {
  ILuint Temp, i;

  if( Mask == 0 ) {
    *ShiftLeft = *ShiftRight = 0;
    return;
  }

  Temp = Mask;
  for( i = 0; i < 32; i++, Temp >>= 1 ) {
    if( Temp & 1 )
      break;
  }
  *ShiftRight = i;

  // Temp is preserved, so use it again:
  for( i = 0; i < 8; i++, Temp >>= 1 ) {
    if( !(Temp & 1) )
      break;
  }
  *ShiftLeft = 8 - i;

  return;
}

static ILboolean iGetOS2Bmp(ILimage* image, OS2_HEAD *Header)
{
  ILuint  PadSize, Read, i, j, k, c;
  ILubyte ByteData;
  SIO *io = &image->io;

  if (Header->cBitCount == 1) {
    if (!iTexImage(image, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
      return IL_FALSE;
    }
    image->Origin = IL_ORIGIN_LOWER_LEFT;

    image->Pal.Palette = (ILubyte*)ialloc(2 * 3);
    if (image->Pal.Palette == NULL) {
      return IL_FALSE;
    }
    image->Pal.PalSize = 2 * 3;
    image->Pal.PalType = IL_PAL_BGR24;

    if (SIOread(io, image->Pal.Palette, 1, 2 * 3) != 6)
      return IL_FALSE;

    PadSize = ((32 - (image->Width % 32)) / 8) % 4;  // Has to truncate.
    SIOseek(io, Header->DataOff, IL_SEEK_SET);

    for (j = 0; j < image->Height; j++) {
      Read = 0;
      for (i = 0; i < image->Width; ) {
        if (SIOread(io, &ByteData, 1, 1) != 1)
          return IL_FALSE;
        Read++;
        k = 128;
        for (c = 0; c < 8; c++) {
          image->Data[j * image->Width + i] = (ByteData & k) ? 1 : 0;
          k >>= 1;
          if (++i >= image->Width)
            break;
        }
      }
      SIOseek(io, PadSize, IL_SEEK_CUR);
    }
    return IL_TRUE;
  }

  if (Header->cBitCount == 4) {
    if (!iTexImage(image, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
      return IL_FALSE;
    }
    image->Origin = IL_ORIGIN_LOWER_LEFT;

    image->Pal.Palette = (ILubyte*)ialloc(16 * 3);
    if (image->Pal.Palette == NULL) {
      return IL_FALSE;
    }
    image->Pal.PalSize = 16 * 3;
    image->Pal.PalType = IL_PAL_BGR24;

    if (SIOread(io, image->Pal.Palette, 1, 16 * 3) != 16*3)
      return IL_FALSE;

    PadSize = ((8 - (image->Width % 8)) / 2) % 4;  // Has to truncate
    SIOseek(io, Header->DataOff, IL_SEEK_SET);

    for (j = 0; j < image->Height; j++) {
      for (i = 0; i < image->Width; i++) {
        if (SIOread(io, &ByteData, 1, 1) != 1)
          return IL_FALSE;
        image->Data[j * image->Width + i] = ByteData >> 4;
        if (++i == image->Width)
          break;
        image->Data[j * image->Width + i] = ByteData & 0x0F;
      }
      SIOseek(io, PadSize, IL_SEEK_CUR);
    }

    return IL_TRUE;
  }

  if (Header->cBitCount == 8) {
    //added this line 2003-09-01...strange no-one noticed before...
    if (!iTexImage(image, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
      return IL_FALSE;

    image->Pal.Palette = (ILubyte*)ialloc(256 * 3);
    if (image->Pal.Palette == NULL) {
      return IL_FALSE;
    }
    image->Pal.PalSize = 256 * 3;
    image->Pal.PalType = IL_PAL_BGR24;

    if (SIOread(io, image->Pal.Palette, 1, 256 * 3) != 256*3)
      return IL_FALSE;
  }
  else { //has to be 24 bpp
    if (!iTexImage(image, Header->cx, Header->cy, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
      return IL_FALSE;
  }
  image->Origin = IL_ORIGIN_LOWER_LEFT;

  SIOseek(io, Header->DataOff, IL_SEEK_SET);

  PadSize = (4 - (image->Bps % 4)) % 4;
  if (PadSize == 0) {
    if (SIOread(io, image->Data, 1, image->SizeOfData) != image->SizeOfData)
      return IL_FALSE;
  }
  else {
    for (i = 0; i < image->Height; i++) {
      if (SIOread(io, image->Data + i * image->Bps, 1, image->Bps) != image->Bps)
        return IL_FALSE;
      SIOseek(io, PadSize, IL_SEEK_CUR);
    }
  }

  return IL_TRUE;
}


static ILboolean prepareBMP(ILimage* image, BMPHEAD * Header, ILubyte bpp, ILuint format)
{
  // Update the current image with the new dimensions
  if (!iTexImage(image, (ILuint)Header->biWidth, (ILuint)abs(Header->biHeight), 1, bpp, format, IL_UNSIGNED_BYTE, NULL)) 
  {
    return IL_FALSE;
  }

  // If the image height is negative, then the image is flipped
  //  (Normal is IL_ORIGIN_LOWER_LEFT)
  if (Header->biHeight < 0) {
    image->Origin = IL_ORIGIN_UPPER_LEFT;
  } else {
    image->Origin = IL_ORIGIN_LOWER_LEFT;
  }

  return IL_TRUE;
}


// Reads an uncompressed monochrome .bmp
static ILboolean ilReadUncompBmp1(ILimage* image, BMPHEAD * Header)
{
  ILuint i, j, k, c, Read;
  ILubyte ByteData, PadSize, Padding[4];
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!prepareBMP(image, Header, 1, IL_COLOUR_INDEX)) 
  {
    return IL_FALSE;
  }

  // Prepare palette
  image->Pal.PalType = IL_PAL_BGR32;
  image->Pal.PalSize = 2 * 4;
  image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
  if (image->Pal.Palette == NULL) {
    return IL_FALSE;
  }

  // Read the palette
  SIOseek(io, sizeof(BMPHEAD), IL_SEEK_SET);
  if (SIOread(io, image->Pal.Palette, 1, image->Pal.PalSize) != image->Pal.PalSize)
    return IL_FALSE;

  // Seek to the data from the "beginning" of the file
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);

  PadSize = ((32 - (image->Width % 32)) / 8) % 4;  // Has to truncate
  for (j = 0; j < image->Height; j++) {
    Read = 0;
    for (i = 0; i < image->Width; ) {
      if (SIOread(io, &ByteData, 1, 1) != 1) {
        return IL_FALSE;
      }
      Read++;
      k = 128;
      for (c = 0; c < 8; c++) {
        image->Data[j * image->Width + i] = 
          (!!(ByteData & k) == 1 ? 1 : 0);
        k >>= 1;
        if (++i >= image->Width)
          break;
      }
    }
    //SIOseek(io, PadSize, IL_SEEK_CUR);
    SIOread(io, Padding, 1, PadSize);
  }

  return IL_TRUE;
}


// Reads an uncompressed .bmp
static ILboolean ilReadUncompBmp4(ILimage* image, BMPHEAD * Header)
{
  ILuint i, j;
  ILubyte ByteData, PadSize, Padding[4];
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!prepareBMP(image, Header,  1, IL_COLOUR_INDEX))
  {
    return IL_FALSE;
  }

  // Prepare palette
  image->Pal.PalType = IL_PAL_BGR32;
  image->Pal.PalSize = Header->biClrUsed ? (ILuint)Header->biClrUsed * 4 : 256 * 4;
      
  if (Header->biBitCount == 4)  // biClrUsed is 0 for 4-bit bitmaps
    image->Pal.PalSize = 16 * 4;
  image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
  if (image->Pal.Palette == NULL) {
    return IL_FALSE;
  }

  // Read the palette
  SIOseek(io, sizeof(BMPHEAD), IL_SEEK_SET);
  if (SIOread(io, image->Pal.Palette, 1, image->Pal.PalSize) != image->Pal.PalSize)
    return IL_FALSE;

  // Seek to the data from the "beginning" of the file
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);

  PadSize = ((8 - (image->Width % 8)) / 2) % 4;  // Has to truncate
  for (j = 0; j < image->Height; j++) {
    for (i = 0; i < image->Width; i++) {
      if (SIOread(io, &ByteData, 1, 1) != 1) {
        return IL_FALSE;
      }
      image->Data[j * image->Width + i] = ByteData >> 4;
      if (++i == image->Width)
        break;
      image->Data[j * image->Width + i] = ByteData & 0x0F;
    }
    SIOread(io, Padding, 1, PadSize);
    //SIOseek(io, PadSize, IL_SEEK_CUR);
  }

  return IL_TRUE;
}


// Reads an uncompressed .bmp
static ILboolean ilReadUncompBmp8(ILimage* image, BMPHEAD * Header)
{
  ILuint PadSize;
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!prepareBMP(image, Header,  1, IL_COLOUR_INDEX))
  {
    return IL_FALSE;
  }

  // Prepare the palette
  image->Pal.PalType = IL_PAL_BGR32;
  image->Pal.PalSize = Header->biClrUsed ? (ILuint)Header->biClrUsed * 4 : 256 * 4;
      
  if (Header->biBitCount == 4)  // biClrUsed is 0 for 4-bit bitmaps
    image->Pal.PalSize = 16 * 4;
  image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
  if (image->Pal.Palette == NULL) {
    return IL_FALSE;
  }

  // Read the palette
  SIOseek(io, sizeof(BMPHEAD), IL_SEEK_SET);
  if (SIOread(io, image->Pal.Palette, 1, image->Pal.PalSize) != image->Pal.PalSize)
    return IL_FALSE;

  // Seek to the data from the "beginning" of the file
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);

  PadSize = (4 - (image->Bps % 4)) % 4;
  if (PadSize == 0) {
    // If the data does not use padding, we can read it immediately
    if (SIOread(io, image->Data, 1, image->SizeOfPlane) != image->SizeOfPlane)
      return IL_FALSE;
  } else {  
    // Remove padding while reading
    ILuint i;
    for (i = 0; i < image->SizeOfPlane; i += image->Bps) {
      ILuint read = SIOread(io, image->Data + i, 1, image->Bps);
      if (read != image->Bps) {
        return IL_FALSE;
      }
      SIOseek(io, PadSize, IL_SEEK_CUR);
    }
  }

  return IL_TRUE;
}


static ILboolean ilReadUncompBmp16(ILimage* image, BMPHEAD * Header)
{
  ILubyte PadSize = 0;
  ILuint rMask, gMask, bMask, rShiftR, gShiftR, bShiftR, rShiftL, gShiftL, bShiftL;
  ILuint inputBps = 2 * (ILuint)Header->biWidth;
  ILubyte* inputScanline = (ILubyte*) ialloc(inputBps);
  ILuint inputOffset = (ILuint)Header->bfDataOff;
  ILuint y;
  SIO *io = &image->io;

  if ((Header->biWidth & 1) != 0) {
    PadSize = 2;
    inputBps += PadSize;
  }

  // Update the current image with the new dimensions, unpack to 24 bpp
  if (!prepareBMP(image, Header,  3, IL_BGR))
  {
    ifree(inputScanline);
    return IL_FALSE;
  }

  if (Header->biCompression == 3)
  {
    // If the bmp uses bitfields, we have to use them to decode the image
    ILuint readOffset = Header->biSize + 14;
    SIOseek(io, readOffset, IL_SEEK_SET); //seek to bitfield data
    SIOread(io, &rMask, 4, 1);
    SIOread(io, &gMask, 4, 1);
    SIOread(io, &bMask, 4, 1);
    UInt(&rMask);
    UInt(&gMask);
    UInt(&bMask);
    GetShiftFromMask(rMask, &rShiftL, &rShiftR);
    GetShiftFromMask(gMask, &gShiftL, &gShiftR);
    GetShiftFromMask(bMask, &bShiftL, &bShiftR);
  } else {
    // Default values if bitfields are not used
    rMask   = 0x7C00;
    gMask   = 0x03E0;
    bMask   = 0x001F;
    rShiftR = 10;
    gShiftR = 5;
    bShiftR = 0;
    rShiftL = 3;
    gShiftL = 3;
    bShiftL = 3;
  }

  // Read pixels
  for (y = 0; y < image->Height; y++) {
    ILuint k = image->Bps * y;
    ILuint x;
    ILuint read;

    SIOseek(io, inputOffset, IL_SEEK_SET);
    read = SIOread(io, inputScanline, 1, inputBps);
    if (read != inputBps) {
      return IL_FALSE;
    }
    inputOffset += inputBps;

    for(x = 0; x < image->Width; x++, k += 3) {
      WORD Read16;
      memcpy(&Read16, inputScanline + 2*x, 2);
      UShort(&Read16);
      image->Data[k    ] = (ILubyte)(((Read16 & bMask) >> bShiftR) << bShiftL);
      image->Data[k + 1] = (ILubyte)(((Read16 & gMask) >> gShiftR) << gShiftL);
      image->Data[k + 2] = (ILubyte)(((Read16 & rMask) >> rShiftR) << rShiftL);
    }
  }

  ifree(inputScanline);

  return IL_TRUE;
}


// Reads an uncompressed .bmp
static ILboolean ilReadUncompBmp24(ILimage* image, BMPHEAD * Header)
{
  ILubyte PadSize;
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!prepareBMP(image, Header,  3, IL_BGR))
  {
    return IL_FALSE;
  }

  // Seek to the data from the "beginning" of the file
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);

  // Read data
  PadSize = (4 - (image->Bps % 4)) % 4;
  if (PadSize == 0) {
    // Read entire data with just one call
    if (SIOread(io, image->Data, 1, image->SizeOfPlane) != image->SizeOfPlane)
      return IL_FALSE;
  } else {  
    // Microsoft requires lines to be padded if the widths aren't multiples of 4
    ILuint i;
    for (i = 0; i < image->SizeOfPlane; i += image->Bps) {
      ILuint read = SIOread(io, image->Data + i, 1, image->Bps);
      if (read != image->Bps) {
        return IL_FALSE;
      }
      SIOseek(io, PadSize, IL_SEEK_CUR);
    }
  }

  return IL_TRUE;
}

// Reads an uncompressed 32-bpp .bmp
static ILboolean ilReadUncompBmp32(ILimage* image, BMPHEAD * Header)
{
  ILuint read;
  DWORD * start, * stop, * pixel;
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!prepareBMP(image, Header,  4, IL_BGRA))
  {
    return IL_FALSE;
  }

  // Read pixel data
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);
  read = SIOread(io, image->Data, 1, image->SizeOfPlane);

  // Convert data: ABGR to BGRA
  // @TODO: bitfields are not supported here yet ... would mean that at least one color channel 
  // could use more than 8 bits precision
  start = (DWORD*)(void*) ilGetData();
  stop = start + image->Width * image->Height;

    for (pixel = start; pixel < stop; ++pixel)
    (*pixel) = (ILuint)(((*pixel) & 255) << 24) + ((*pixel) >> 8);

  if ((ILuint)read == image->SizeOfPlane) {
    return IL_TRUE;
  } else {
    return IL_FALSE;
  }
}

// Reads an uncompressed .bmp
static ILboolean ilReadUncompBmp(ILimage* image, BMPHEAD * header)
{
  // A height of 0 is illegal
  if (header->biHeight == 0) {
    iSetError(IL_ILLEGAL_FILE_VALUE);
    if (image->Pal.Palette)
      ifree(image->Pal.Palette);
    return IL_FALSE;
  }

  switch (header->biBitCount)
  {
    case 1:       return ilReadUncompBmp1(image, header);
    case 4:       return ilReadUncompBmp4(image, header);
    case 8:       return ilReadUncompBmp8(image, header);
    case 16:      return ilReadUncompBmp16(image, header);
    case 24:      return ilReadUncompBmp24(image, header);
    case 32:      return ilReadUncompBmp32(image, header);
    default:      iSetError(IL_ILLEGAL_FILE_VALUE);
                  return IL_FALSE;
  }
}

static ILboolean ilReadRLE8Bmp(ILimage* image, BMPHEAD *Header)
{
  ILubyte Bytes[2];
  size_t  offset = 0, count, endOfLine = (ILuint)Header->biWidth;
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!iTexImage(image, (ILuint)Header->biWidth, (ILuint)abs(Header->biHeight), 1, 1, 0, IL_UNSIGNED_BYTE, NULL))
    return IL_FALSE;

  image->Origin = IL_ORIGIN_LOWER_LEFT;

  // A height of 0 is illegal
  if (Header->biHeight == 0)
    return IL_FALSE;

  image->Format = IL_COLOUR_INDEX;
  image->Pal.PalType = IL_PAL_BGR32;
  image->Pal.PalSize = Header->biClrUsed * 4;  // 256 * 4 for most images
  if (image->Pal.PalSize == 0)
    image->Pal.PalSize = 256 * 4;
  image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
  if (image->Pal.Palette == NULL)
    return IL_FALSE;

  // If the image height is negative, then the image is flipped
  //  (Normal is IL_ORIGIN_LOWER_LEFT)
  image->Origin = Header->biHeight < 0 ?
     IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;
  
  // Read the palette
  SIOseek(io, sizeof(BMPHEAD), IL_SEEK_SET);
  if (SIOread(io, image->Pal.Palette, image->Pal.PalSize, 1) != 1)
    return IL_FALSE;

  // Seek to the data from the "beginning" of the file
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);

  while (offset < image->SizeOfData) {
    if (SIOread(io, Bytes, sizeof(Bytes), 1) != 1)
      return IL_FALSE;
    if (Bytes[0] == 0x00) {  // Escape sequence
      switch (Bytes[1])
      {
        case 0x00:  // End of line
          offset = endOfLine;
          endOfLine += image->Width;
          break;
        case 0x01:  // End of bitmap
          offset = image->SizeOfData;
          break;
        case 0x2:
          if (SIOread(io, Bytes, sizeof(Bytes), 1) != 1)
            return IL_FALSE;
          offset += Bytes[0] + Bytes[1] * image->Width;
          endOfLine += Bytes[1] * image->Width;
          break;
        default:
          count = IL_MIN(Bytes[1], image->SizeOfData-offset);
          if (SIOread(io, image->Data + offset, (ILuint)count, 1) != 1)
            return IL_FALSE;
          offset += count;
          if ((count & 1) == 1)  // Must be on a word boundary
            if (SIOread(io, Bytes, 1, 1) != 1)
              return IL_FALSE;
          break;
      }
    } else {
      count = IL_MIN (Bytes[0], image->SizeOfData-offset);
      memset(image->Data + offset, Bytes[1], count);
      offset += count;
    }
  }
  return IL_TRUE;
}


static ILboolean ilReadRLE4Bmp(ILimage* image, BMPHEAD *Header)
{
  ILubyte Bytes[2];
  ILuint  i;
  size_t  offset = 0, count, endOfLine = (ILuint)Header->biWidth;
  SIO *io = &image->io;

  // Update the current image with the new dimensions
  if (!iTexImage(image, (ILuint)Header->biWidth, (ILuint)abs(Header->biHeight), 1, 1, 0, IL_UNSIGNED_BYTE, NULL))
    return IL_FALSE;
  image->Origin = IL_ORIGIN_LOWER_LEFT;

  // A height of 0 is illegal
  if (Header->biHeight == 0) {
    iSetError(IL_ILLEGAL_FILE_VALUE);
    return IL_FALSE;
  }

  image->Format = IL_COLOUR_INDEX;
  image->Pal.PalType = IL_PAL_BGR32;
  image->Pal.PalSize = 16 * 4; //Header->biClrUsed * 4;  // 16 * 4 for most images
  image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
  if (image->Pal.Palette == NULL)
    return IL_FALSE;

  // If the image height is negative, then the image is flipped
  //  (Normal is IL_ORIGIN_LOWER_LEFT)
  image->Origin = Header->biHeight < 0 ?
     IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;

  // Read the palette
  SIOseek(io, sizeof(BMPHEAD), IL_SEEK_SET);

  if (SIOread(io, image->Pal.Palette, image->Pal.PalSize, 1) != 1)
    return IL_FALSE;

  // Seek to the data from the "beginning" of the file
  SIOseek(io, Header->bfDataOff, IL_SEEK_SET);

  while (offset < image->SizeOfData) {
    if (SIOread(io, &Bytes[0], sizeof(Bytes), 1) != 1)
      return IL_FALSE;
    if (Bytes[0] == 0x0) {        // Escape sequence
      int align;
      switch (Bytes[1]) {
        case 0x0:  // End of line
          offset = endOfLine;
          endOfLine += image->Width;
          break;

        case 0x1:  // End of bitmap
          offset = image->SizeOfData;
          break;

        case 0x2:
          if (SIOread(io, &Bytes[0], sizeof(Bytes), 1) != 1)
            return IL_FALSE;
          offset += Bytes[0] + Bytes[1] * image->Width;
          endOfLine += Bytes[1] * image->Width;
          break;

        default:   // Run of pixels
          count = IL_MIN (Bytes[1], image->SizeOfData-offset);

          for (i = 0; i < count; i++) {
            int byte;

            if ((i & 0x01) == 0) {
              if (SIOread(io, &Bytes[0], sizeof(Bytes[0]), 1) != 1)
                return IL_FALSE;
              byte = (Bytes[0] >> 4);
            }
            else
              byte = (Bytes[0] & 0x0F);
            image->Data[offset++] = (ILubyte)byte;
          }

          align = Bytes[1] % 4;

          if (align == 1 || align == 2) // Must be on a word boundary
            if (SIOread(io, &Bytes[0], sizeof(Bytes[0]), 1) != 1)
              return IL_FALSE;
      }
    } else {
         count = IL_MIN (Bytes[0], image->SizeOfData-offset);
         Bytes[0] = (Bytes[1] >> 4);
      Bytes[1] &= 0x0F;
      for (i = 0; i < count; i++)
        image->Data[offset++] = Bytes [i & 1];
    }
  }

   return IL_TRUE;
}


// Internal function used to load the .bmp.
static ILboolean iLoadBitmapInternal(ILimage* image)
{
  BMPHEAD   Header;
  OS2_HEAD  Os2Head;
  ILboolean bBitmap;
  SIO *io = &image->io;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  iGetBmpHead(io, &Header);
  if (!iCheckBmp(&Header)) {
    iGetOS2Head(io, &Os2Head);
    if (!iCheckOS2(&Os2Head)) {
      iSetError(IL_INVALID_FILE_HEADER);
      return IL_FALSE;
    }
    else {
      return iGetOS2Bmp(image, &Os2Head);
    }
  }

  // Don't know what to do if it has more than one plane...
  if (Header.biPlanes != 1) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  switch (Header.biCompression)
  {
    case 0: // No compression
    case 3: // BITFIELDS compression is handled in 16 bit
            // and 32 bit code in ilReadUncompBmp()
      bBitmap = ilReadUncompBmp(image, &Header);
      break;
    case 1:  // RLE 8-bit / pixel (BI_RLE8)
      bBitmap = ilReadRLE8Bmp(image, &Header);
      break;
    case 2:  // RLE 4-bit / pixel (BI_RLE4)
      bBitmap = ilReadRLE4Bmp(image, &Header);
      break;

    default:
      iSetError(IL_INVALID_FILE_HEADER);
      return IL_FALSE;
  }

  return bBitmap;
}


// Internal function used to save the .bmp.
static ILboolean iSaveBitmapInternal(ILimage* image)
{
  ILboolean compress_rle8 = ilIsEnabled(IL_BMP_RLE);
  //int compress_rle8 = IL_FALSE; // disabled BMP RLE compression. broken
  ILuint  FileSize, i, PadSize, Padding = 0;
  ILuint  FileSizePos = 0;
  ILimage *TempImage = NULL;
  ILpal *TempPal;
  ILubyte *TempData;
  SIO *io = &image->io;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (compress_rle8 == IL_TRUE)
  {
    TempImage = iConvertImage(image, IL_COLOR_INDEX, IL_UNSIGNED_BYTE);
    if (TempImage == NULL)
      return IL_FALSE;
    TempPal = iConvertPal(&TempImage->Pal, IL_PAL_BGR32);
    if (TempPal == NULL)
    {
      iCloseImage(TempImage);
      return IL_FALSE;
    }
  } else {
    if((image->Format == IL_LUMINANCE) && (image->Pal.Palette == NULL))
    {
      // For luminance images it is necessary to generate a grayscale BGR32
      //  color palette.  Could call iConvertImage(..., IL_COLOR_INDEX, ...)
      //  to generate an RGB24 palette, followed by iConvertPal(..., IL_PAL_BGR32),
      //  to convert the palette to BGR32, but it seemed faster to just
      //  explicitely generate the correct palette.
      
      image->Pal.PalSize = 256*4;
      image->Pal.PalType = IL_PAL_BGR32;
      image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
      
      // Generate grayscale palette
      for (i = 0; i < 256; i++)
      {
        image->Pal.Palette[i * 4]     =
        image->Pal.Palette[i * 4 + 1] =
        image->Pal.Palette[i * 4 + 2] = (ILubyte)i;
        image->Pal.Palette[i * 4 + 3] = 0;
      }
    }
    
    // If the current image has a palette, take care of it
    TempPal = &image->Pal;
    if( image->Pal.PalSize && image->Pal.Palette && image->Pal.PalType != IL_PAL_NONE ) {
      // If the palette in .bmp format, write it directly
      if (image->Pal.PalType == IL_PAL_BGR32) {
        TempPal = &image->Pal;
      } else {
        TempPal = iConvertPal(&image->Pal, IL_PAL_BGR32);
        if (TempPal == NULL) {
          return IL_FALSE;
        }
      }
    }

    //Changed 20040923: moved this block above writing of
    //BITMAPINFOHEADER, so that the written header refers to
    //TempImage instead of the original image
    
    if ((image->Format != IL_BGR) && (image->Format != IL_BGRA) && 
        (image->Format != IL_COLOUR_INDEX) && (image->Format != IL_LUMINANCE)) {
      if (image->Format == IL_RGBA) {
        TempImage = iConvertImage(image, IL_BGRA, IL_UNSIGNED_BYTE);
      } else {
        TempImage = iConvertImage(image, IL_BGR, IL_UNSIGNED_BYTE);
      }
      if (TempImage == NULL)
        return IL_FALSE;
    } else if (image->Bpc > 1) {
      TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
      if (TempImage == NULL)
        return IL_FALSE;
    } else {
      TempImage = image;
    }

  }

  if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
    TempData = iGetFlipped(TempImage);
    if (TempData == NULL) {
      iCloseImage(TempImage);
      return IL_FALSE;
    }
  } else {
    TempData = TempImage->Data;
  }
  
  SIOputc(io, 'B');  // Comprises the
  SIOputc(io, 'M');  //  "signature"
  
  SaveLittleUInt(io, 0);  // Will come back and change later in this function (filesize)
  SaveLittleUInt(io, 0);  // Reserved

  SaveLittleUInt(io, 54 + TempPal->PalSize);  // Offset of the data

  SaveLittleUInt(io, 0x28);  // Header size
  SaveLittleUInt(io, TempImage->Width);

  // Removed because some image viewers don't like the negative height.
  // even if it is standard. @TODO should be enabled or disabled
  // usually enabled.
  /*if (image->Origin == IL_ORIGIN_UPPER_LEFT)
    SaveLittleInt(io, -(ILint)image->Height);
  else*/
    SaveLittleUInt(io, TempImage->Height);

  SaveLittleUShort(io, 1);  // Number of planes
  SaveLittleUShort(io, (ILushort)((ILushort)TempImage->Bpp << 3));  // Bpp
  if( compress_rle8 == IL_TRUE ) {
    SaveLittleInt(io, 1); // rle8 compression
  } else {
    SaveLittleInt(io, 0);
  }

  FileSizePos = SIOtell(io);
  SaveLittleUInt(io, 0);  // Size of image (Obsolete)
  SaveLittleInt(io, 3780); // horiz. px/m, 3780 ~96dpi (Obsolete)
  SaveLittleInt(io, 3780); // vert.  px/m (Obsolete)

  if (TempImage->Pal.PalType != IL_PAL_NONE) {
    SaveLittleInt(io, ilGetInteger(IL_PALETTE_NUM_COLS));  // Num colours used
  } else {
    SaveLittleInt(io, 0);
  }
  SaveLittleInt(io, 0);  // Important colour (none)

  SIOwrite(io, TempPal->Palette, 1, TempPal->PalSize);
  
  if( compress_rle8 == IL_TRUE ) {
    ILubyte *Dest = (ILubyte*)ialloc((ILuint)(TempImage->SizeOfData*2));
    ILuint ImagePos = SIOtell(io);
    FileSize = iRleCompress(TempImage->Data,TempImage->Width,TempImage->Height,
            TempImage->Depth,1,Dest,IL_BMPCOMP,NULL);

    SIOseek(io, FileSizePos, IL_SEEK_SET);
    SaveLittleUInt(io, FileSize);
    SIOseek(io, ImagePos, IL_SEEK_SET);
    SIOwrite(io, Dest, 1, FileSize);
  } else {
    PadSize = (4 - (TempImage->Bps % 4)) % 4;

    // No padding, so write data directly.
    if (PadSize == 0) {
      SIOwrite(io, TempData, 1, TempImage->SizeOfPlane);
    } else {  // Odd width, so we must pad each line.
      for (i = 0; i < TempImage->SizeOfPlane; i += TempImage->Bps) {
        SIOwrite(io, TempData + i, 1, TempImage->Bps); // Write data
        SIOwrite(io, &Padding, 1, PadSize); // Write pad byte(s)
      }
    }
  }
  
  // Write the filesize
  FileSize = SIOtell(io);
  SIOseek(io, 2, IL_SEEK_SET);
  SaveLittleUInt(io, FileSize);

  if (TempPal != &image->Pal) {
    ifree(TempPal->Palette);
    ifree(TempPal);
  }
  if (TempData != TempImage->Data)
    ifree(TempData);
  if (TempImage != image)
    iCloseImage(TempImage);
  
  SIOseek(io, FileSize, IL_SEEK_SET);
  
  return IL_TRUE;
}

static ILconst_string iFormatExtsBMP[] = { 
  IL_TEXT("bmp"), 
  IL_TEXT("dib"), 
  NULL 
};

ILformat iFormatBMP = { 
  /* .Validate = */ iIsValidBmp, 
  /* .Load     = */ iLoadBitmapInternal, 
  /* .Save     = */ iSaveBitmapInternal, 
  /* .Exts     = */ iFormatExtsBMP
};

#endif//IL_NO_BMP
