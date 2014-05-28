//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/06/2009
//
// Filename: src-IL/src/il_sun.c
//
// Description: Reads from a Sun RAS file.  Specifications available from
//        http://www.fileformat.info/format/sunraster/egff.htm.
//
//-----------------------------------------------------------------------------


// bg, 2014-05-09:
// There appears to be some confusion on when scanlines need to be aligned to
// 16-bit boundaries. GIMP assumes this for loading and saving, but IrfanView
// shows these images as corrupted. Only images with an odd width (w % 2 != 0)
// are affected. May also depend on bpp.


#include "il_internal.h"
#ifndef IL_NO_SUN
#include "il_bits.h"

static ILboolean  iLoadSunInternal(ILimage* image);

#include "pack_push.h"
typedef struct SUNHEAD
{
  ILuint MagicNumber;      // Magic (identification) number
  ILuint Width;            // Width of image in pixels
  ILuint Height;           // Height of image in pixels
  ILuint BitsPerPixel;            // Number of bits per pixel
  ILuint DataSize;         // Size of decoded image data in bytes
  ILuint Type;             // Type of raster file
  ILuint ColorMapType;     // Type of color map
  ILuint ColorMapLength;   // Size of the color map in bytes
} SUNHEAD;
#include "pack_pop.h"

// Data storage types
#define IL_SUN_OLD      0x00
#define IL_SUN_STANDARD 0x01
#define IL_SUN_BYTE_ENC 0x02
#define IL_SUN_RGB      0x03
#define IL_SUN_TIFF     0x04
#define IL_SUN_IFF      0x05
#define IL_SUN_EXPER    0xFFFF  // Experimental, not supported.

// Colormap types
#define IL_SUN_NO_MAP   0x00
#define IL_SUN_RGB_MAP  0x01
#define IL_SUN_RAW_MAP  0x02

// Internal function used to get the Sun header from the current file.
static ILuint iGetSunHead(SIO* io, SUNHEAD *Header)
{
  ILuint read = SIOread(io, Header, 1, sizeof(SUNHEAD));

  BigUInt(&Header->MagicNumber);
  BigUInt(&Header->Width);
  BigUInt(&Header->Height);
  BigUInt(&Header->BitsPerPixel);
  BigUInt(&Header->DataSize);
  BigUInt(&Header->Type);
  BigUInt(&Header->ColorMapType);
  BigUInt(&Header->ColorMapLength);

  return read;
}

// Internal function used to check if the HEADER is a valid SUN header.
static ILboolean iCheckSun(SUNHEAD *Header) {
  if (Header->MagicNumber != 0x59A66A95)  // Magic number is always 0x59A66A95.
    return IL_FALSE;
  if (Header->Width == 0 || Header->Height == 0)  // 0 dimensions are meaningless.
    return IL_FALSE;
  // These are the only valid depths that I know of.
  if (Header->BitsPerPixel != 1 && Header->BitsPerPixel != 8 && Header->BitsPerPixel != 24 && Header->BitsPerPixel != 32)
    return IL_FALSE;
  if (Header->Type > IL_SUN_RGB)  //@TODO: Support further types.
    return IL_FALSE;
  if (Header->ColorMapType > IL_SUN_RGB_MAP)  //@TODO: Find out more about raw map.
    return IL_FALSE;
  // Color map cannot be 0 if there is a map indicated.
  if (Header->ColorMapType > IL_SUN_NO_MAP && Header->ColorMapLength == 0)
    return IL_FALSE;
  //@TODO: These wouldn't make sense.  Are they valid somehow?  Find out...
  if ((Header->BitsPerPixel == 1 || Header->BitsPerPixel == 32) && Header->Type == IL_SUN_BYTE_ENC)
    return IL_FALSE;

  return IL_TRUE;
}

// Internal function to get the header and check it.
static ILboolean iIsValidSun(SIO* io) {
  SUNHEAD Head;
  ILint read = iGetSunHead(io, &Head);
  SIOseek(io, -read, IL_SEEK_CUR);

  if (read == sizeof(Head))
    return iCheckSun(&Head);
  else
    return IL_FALSE;
}

/*
static ILuint iSunGetRle(ILimage* image, ILubyte *Data, ILuint Length) {
  ILuint  i = 0, j;
  ILubyte Flag, Value;
  ILuint  Count;

  SIO *io = &image->io;

  for (i = 0; i < Length; ) {
    Flag = SIOgetc(io);
    if (Flag == 0x80) {  // Run follows (or 1 byte of 0x80)
      Count = SIOgetc(io);
      if (Count == 0) {  // 1 pixel of value (0x80)
        *Data = 0x80;
        Data++;
        i++;
      }
      else {  // Here we have a run.
        Value = SIOgetc(io);
        Count++;  // Should really be Count+1
        for (j = 0; j < Count && i + j < Length; j++, Data++) {
          *Data = Value;
        }
        i += Count;
      }
    }
    else {  // 1 byte of this value (cannot be 0x80)
      *Data = Flag;
      Data++;
      i++;
    }
  }

  return i;
}
*/
static void writeSunPixels(ILimage* image, ILuint *dataOffset, ILubyte value, int count) {
  if (*dataOffset < image->SizeOfData) {
    if (*dataOffset + count > image->SizeOfData)
      count = image->SizeOfData - *dataOffset;
    memset(&image->Data[*dataOffset], value, count);
    *dataOffset += count;
  }
}

// iSunGetRle assumes that data runs do not cross scanlines, but samples and documentation
// indicate otherwise
static void iSunDecodeRle(ILimage* image) {
  ILuint dataOffset = 0;
  SIO *io = &image->io;

  while (dataOffset < image->SizeOfData && !SIOeof(io)) {
    ILubyte Flag = SIOgetc(io);
    if (Flag == 0x80) {
      ILuint count = SIOgetc(io);
      if (count == 0) {
        // Output 0x80 once (input values of 0x80 are encoded as 0x8000)
        writeSunPixels(image, &dataOffset, 0x80, 1);
      } else {
        // Here we have a run.
        ILubyte value = SIOgetc(io);
        writeSunPixels(image, &dataOffset, value, count+1);
      }
    } else {
      // Copy 1 byte to output
      writeSunPixels(image, &dataOffset, Flag, 1);
    }
  }
}

static void iSunPrepareImageBuffer(ILimage* image, SUNHEAD* header) {
  if ((image->Width & 1) == 1 
  &&  (image->Bpp == 1 || image->Bpp == 3)
  &&  (header->Type == IL_SUN_BYTE_ENC))
  {
    ++image->Bps;
    image->SizeOfData = image->Bps * image->Height;
    image->SizeOfPlane = image->SizeOfPlane * image->Depth;
    ifree(image->Data);
    ialloc(image->SizeOfData);
  }
}

static ILboolean iLoadSunInternal(ILimage* image) {
  SUNHEAD Header;
  BITFILE *File;
  ILuint  i, Padding;
  ILubyte PaddingData[16];
  ILubyte* buf = NULL;
  ILuint outputOffset;

  if (image == NULL) {
    ilSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  SIO *io = &image->io;

  if (iGetSunHead(io, &Header) < sizeof(Header)) {
    ilSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  if (!iCheckSun(&Header)) {
    ilSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  switch (Header.BitsPerPixel) {
    case 1:  //@TODO: Find a file to test this on.
      File = bitfile(io);
      if (File == NULL)
        return IL_FALSE;

      if (!ilTexImage_(image, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
        return IL_FALSE;

      if (Header.ColorMapLength != 0) {
        // Data should be an index into the color map, but the color map should only be RGB (6 bytes, 2 entries).
        if (Header.ColorMapLength != 6) {
          ilSetError(IL_INVALID_FILE_HEADER);
          return IL_FALSE;
        }
      }
      image->Pal.Palette = (ILubyte*)ialloc(6);  // Just need 2 entries in the color map.
      if (Header.ColorMapLength == 0) {  // Create the color map
        image->Pal.Palette[0] = 0x00;  // Entry for black
        image->Pal.Palette[1] = 0x00;
        image->Pal.Palette[2] = 0x00;
        image->Pal.Palette[3] = 0xFF;  // Entry for white
        image->Pal.Palette[4] = 0xFF;
        image->Pal.Palette[5] = 0xFF;
      }
      else {
        if (SIOread(io, image->Pal.Palette, 1, 6) != 6) { // Read in the color map.
          ilSetError(IL_INVALID_FILE_HEADER);
          return IL_FALSE;
        }
      }
      image->Pal.PalSize = 6;
      image->Pal.PalType = IL_PAL_RGB24;

      Padding = (16 - (image->Width % 16)) % 16;  // Has to be aligned on a 16-bit boundary.  The rest is padding.

      // Reads the bits
      for (i = 0; i < image->Height; i++) {
        bread(&image->Data[image->Width * i], 1, image->Width, File);
        //bseek(File, BitPadding, IL_SEEK_CUR);  //@TODO: This function does not work correctly.
        bread(PaddingData, 1, Padding, File);  // Skip padding bits.
      }
      break;


    case 8:
      if (Header.ColorMapType == IL_SUN_NO_MAP) {  // Greyscale image
        if (!ilTexImage_(image, Header.Width, Header.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
      }
      else {  // Colour-mapped image
        if (!ilTexImage_(image, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
        image->Pal.Palette = (ILubyte*)ialloc(Header.ColorMapLength);  // Allocate color map.
        if (image->Pal.Palette == NULL)
          return IL_FALSE;

        ILubyte* sunPalette = (ILubyte*) ialloc(Header.ColorMapLength);
        if (SIOread(io, sunPalette, 1, Header.ColorMapLength) != Header.ColorMapLength) {  // Read color map.
          ilSetError(IL_FILE_READ_ERROR);
          return IL_FALSE;
        }
        int colCount = Header.ColorMapLength / 3;
        ILubyte* r = sunPalette;
        ILubyte* g = &sunPalette[Header.ColorMapLength / 3];
        ILubyte* b = &sunPalette[2*colCount];

        int i;
        for (i = 0; i < colCount; ++i) {
          image->Pal.Palette[3*i] = r[i];
          image->Pal.Palette[3*i+1] = g[i];
          image->Pal.Palette[3*i+2] = b[i];
        }
        ifree(sunPalette);

        image->Pal.PalSize = Header.ColorMapLength;
        image->Pal.PalType = IL_PAL_RGB24;
      }

      if (Header.Type != IL_SUN_BYTE_ENC) {  // Regular uncompressed image data
        Padding = (2 - (image->Bps % 2)) % 2;  // Must be padded on a 16-bit boundary (2 bytes)
        for (i = 0; i < Header.Height; i++) {
          SIOread(io, image->Data + i * Header.Width, 1, image->Bps);
          if (Padding)  // Only possible for padding to be 0 or 1.
            SIOgetc(io);
        }
      }
      else {  // RLE image data
        iSunDecodeRle(image);
      }
      break;

    case 24:
      if (Header.ColorMapLength > 0)  // Ignore any possible colormaps.
        SIOseek(io, Header.ColorMapLength, IL_SEEK_CUR);

      if (Header.Type == IL_SUN_RGB) {
        if (!ilTexImage_(image, Header.Width, Header.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
      }
      else {
        if (!ilTexImage_(image, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
      }
      iSunPrepareImageBuffer(image, &Header);

      if (Header.Type != IL_SUN_BYTE_ENC) {  // Regular uncompressed image data
        Padding = (2 - (image->Bps % 2)) % 2;  // Must be padded on a 16-bit boundary (2 bytes)
        for (i = 0; i < Header.Height; i++) {
          SIOread(io, image->Data + i * Header.Width * 3, 1, image->Bps);
          if (Padding)  // Only possible for padding to be 0 or 1.
            SIOgetc(io);
        }
      }
      else {  // RLE image data
        iSunDecodeRle(image);
      }

      break;

    case 32:
      if (Header.ColorMapLength > 0)  // Ignore any possible colormaps.
        SIOseek(io, Header.ColorMapLength, IL_SEEK_CUR);

      if (Header.Type == IL_SUN_RGB) {
        if (!ilTexImage_(image, Header.Width, Header.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
      }
      else {
        if (!ilTexImage_(image, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
          return IL_FALSE;
      }

      // There is no padding at the end of each scanline.
      buf = (ILubyte*) ialloc(4*image->Width);
      outputOffset = 0;

      ILuint y;
      for (y = 0; y < Header.Height; y++) {
        ILuint inputOffset = 0;
        ILuint read = SIOread(io, buf, 1, 4*image->Width);
        while (inputOffset < read) {
          image->Data[outputOffset]   = buf[inputOffset];
          image->Data[outputOffset+1] = buf[inputOffset+1];
          image->Data[outputOffset+2] = buf[inputOffset+2];
          inputOffset += 4;
          outputOffset += 3;
        }
      }
      ifree(buf);
      break;

    default:  // Should have already been checked with iGetSunHead.
      return IL_FALSE;
  }

  image->Origin = IL_ORIGIN_UPPER_LEFT;
  return IL_TRUE;
}

ILconst_string iFormatExtsSUN[] = { 
  IL_TEXT("sun"), 
  IL_TEXT("ras"), 
  IL_TEXT("im1"), 
  IL_TEXT("im8"), 
  IL_TEXT("im24"),
  IL_TEXT("im32"), 
  NULL 
};

ILformat iFormatSUN = { 
  .Validate = iIsValidSun, 
  .Load     = iLoadSunInternal, 
  .Save     = NULL, 
  .Exts     = iFormatExtsSUN
};

#endif//IL_NO_SUN

