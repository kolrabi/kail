//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_targa.c
//
// Description: Reads from and writes to a Targa (.tga) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_TGA
#include "il_targa.h"
#include <time.h>  
#include <string.h>
#include "il_manip.h"
#include "il_endian.h"

// Internal functions
static ILboolean  iCheckTarga(TARGAHEAD *Header);
static ILboolean  iLoadTargaInternal(ILimage* image);
static ILboolean  iSaveTargaInternal(ILimage* image);
static ILboolean  iReadBwTga(ILimage* image, TARGAHEAD *Header);
static ILboolean  iReadColMapTga(ILimage* image, TARGAHEAD *Header);
static ILboolean  iReadUnmapTga(ILimage* image, TARGAHEAD *Header);
static ILboolean  iUncompressTgaData(ILimage *Image);
static ILboolean  i16BitTarga(ILimage *Image);
static void       iGetDateTime(ILshort *Month, ILshort *Day, ILshort *Yr, ILshort *Hr, ILshort *Min, ILshort *Sec);

static ILint iGetTgaHead(SIO* io, TARGAHEAD *Header) {
  return SIOread(io, Header, 1, sizeof(TARGAHEAD));
}

// Internal function to get the header and check it.
static ILboolean iIsValidTarga(SIO* io) {
  TARGAHEAD Head;
  ILint read = iGetTgaHead(io, &Head);

  SIOseek(io, -read, IL_SEEK_CUR);

  if (read == sizeof(Head)) 
    return iCheckTarga(&Head);
  else
    return IL_FALSE;
}


// Internal function used to check if the HEADER is a valid Targa header.
static ILboolean iCheckTarga(TARGAHEAD *Header) {
  if (Header->Width == 0 || Header->Height == 0)
    return IL_FALSE;
  if ( Header->Bpp != 8 && Header->Bpp != 15 && Header->Bpp != 16 
    && Header->Bpp != 24 && Header->Bpp != 32 )
    return IL_FALSE;
  if (Header->ImageDesc & BIT(4)) // Supposed to be set to 0
    return IL_FALSE;
  
  // check type (added 20040218)
  if (Header->ImageType   != TGA_NO_DATA
     && Header->ImageType != TGA_COLMAP_UNCOMP
     && Header->ImageType != TGA_UNMAP_UNCOMP
     && Header->ImageType != TGA_BW_UNCOMP
     && Header->ImageType != TGA_COLMAP_COMP
     && Header->ImageType != TGA_UNMAP_COMP
     && Header->ImageType != TGA_BW_COMP)
    return IL_FALSE;
  
  // Doesn't work well with the bitshift so change it.
  if (Header->Bpp == 15)
    Header->Bpp = 16;
  
  return IL_TRUE;
}

// Internal function used to load the Targa.
static ILboolean iLoadTargaInternal(ILimage* image) {
  TARGAHEAD Header;
  TARGAFOOTER Footer;
  TARGAEXT  Ext;
  ILboolean bTarga;
  ILenum    iOrigin;
  char      ID[256];
  SIO *io;
  
  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;
  
  if (!iGetTgaHead(&image->io, &Header))
    return IL_FALSE;
  if (!iCheckTarga(&Header)) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }
  
  switch (Header.ImageType)
  {
    case TGA_NO_DATA:
      iSetError(IL_ILLEGAL_FILE_VALUE);
      return IL_FALSE;
    case TGA_COLMAP_UNCOMP:
    case TGA_COLMAP_COMP:
      bTarga = iReadColMapTga(image, &Header);
      break;
    case TGA_UNMAP_UNCOMP:
    case TGA_UNMAP_COMP:
      bTarga = iReadUnmapTga(image, &Header);
      break;
    case TGA_BW_UNCOMP:
    case TGA_BW_COMP:
      bTarga = iReadBwTga(image, &Header);
      break;
    default:
      iSetError(IL_ILLEGAL_FILE_VALUE);
      return IL_FALSE;
  }

  if (bTarga==IL_FALSE)
    return IL_FALSE;
  
  // @JASON Extra Code to manipulate the image depending on
  // the Image Descriptor's origin bits.
  iOrigin = Header.ImageDesc & IMAGEDESC_ORIGIN_MASK;
  
  switch (iOrigin)
  {
    case IMAGEDESC_TOPLEFT:
      image->Origin = IL_ORIGIN_UPPER_LEFT;
      break;
      
    case IMAGEDESC_TOPRIGHT:
      image->Origin = IL_ORIGIN_UPPER_LEFT;
      iMirrorImage(image);
      break;
      
    case IMAGEDESC_BOTLEFT:
      image->Origin = IL_ORIGIN_LOWER_LEFT;
      break;
      
    case IMAGEDESC_BOTRIGHT:
      image->Origin = IL_ORIGIN_LOWER_LEFT;
      iMirrorImage(image);
      break;
  }

  SIOread(io, ID, 1, Header.IDLen);
  ID[Header.IDLen] = 0;
  iSetString(IL_TGA_ID_STRING, ID); // FIXME: put this in meta tags

  imemclear(&Footer, sizeof(Footer));
  SIOseek(io, sizeof(TARGAFOOTER), IL_SEEK_END);
  if (!SIOread(io, &Footer, sizeof(Footer), 1) ||
      memcmp(Footer.Signature, "TRUEVISION-XFILE", 16)) {
    // not a tga 2.0 file, we're done
    return IL_TRUE;
  }

  UInt(&Footer.DevDirOff);
  UInt(&Footer.ExtOff);

  SIOseek(io, Footer.ExtOff, IL_SEEK_SET);
  SIOread(io, &Ext, 1, sizeof(Ext));

  iSetString(IL_META_ARTIST, Ext.AuthName);
  iSetString(IL_META_USER_COMMENT, Ext.AuthComments);

  // TODO: other metadata?

  return IL_TRUE;
}

static ILboolean iReadColMapTga(ILimage* image, TARGAHEAD *Header) {
  char    ID[255];
  ILuint    i;
  ILushort  Pixel;

  SIO *io = &image->io;
  
  if (SIOread(io, ID, 1, Header->IDLen) != Header->IDLen)
    return IL_FALSE;
  
  if (!iTexImage(image, Header->Width, Header->Height, 1, (ILubyte)(Header->Bpp >> 3), 0, IL_UNSIGNED_BYTE, NULL)) {
    return IL_FALSE;
  }
  if (image->Pal.Palette && image->Pal.PalSize)
    ifree(image->Pal.Palette);
  
  image->Format = IL_COLOUR_INDEX;
  image->Pal.PalSize = Header->ColMapLen * (Header->ColMapEntSize >> 3);
  
  switch (Header->ColMapEntSize)
  {
    case 16:
      image->Pal.PalType = IL_PAL_BGRA32;
      image->Pal.PalSize = Header->ColMapLen * 4;
      break;
    case 24:
      image->Pal.PalType = IL_PAL_BGR24;
      break;
    case 32:
      image->Pal.PalType = IL_PAL_BGRA32;
      break;
    default:
      // Should *never* reach here
      iSetError(IL_ILLEGAL_FILE_VALUE);
      return IL_FALSE;
  }
  
  image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
  if (image->Pal.Palette == NULL) {
    return IL_FALSE;
  }
  
  // Do we need to do something with FirstEntry?  Like maybe:
  //  image->io.read(image->io.handle, Image->Pal + Targa->FirstEntry, 1, Image->Pal.PalSize);  ??
  if (Header->ColMapEntSize != 16)
  {
    if (SIOread(io, image->Pal.Palette, 1, image->Pal.PalSize) != image->Pal.PalSize)
      return IL_FALSE;
  }
  else {
    // 16 bit palette, so we have to break it up.
    for (i = 0; i < image->Pal.PalSize; i += 4)
    {
      Pixel = GetBigUShort(io);
      if (SIOeof(io))
        return IL_FALSE;
      image->Pal.Palette[3] = (Pixel & 0x8000) >> 12;
      image->Pal.Palette[0] = (Pixel & 0xFC00) >> 7;
      image->Pal.Palette[1] = (Pixel & 0x03E0) >> 2;
      image->Pal.Palette[2] = (Pixel & 0x001F) << 3;
    }
  }
  
  if (Header->ImageType == TGA_COLMAP_COMP) {
    if (!iUncompressTgaData(image)) {
      return IL_FALSE;
    }
  } else{
    if (SIOread(io, image->Data, 1, image->SizeOfData) != image->SizeOfData) {
      return IL_FALSE;
    }
  }
  
  return IL_TRUE;
}

static ILboolean iReadUnmapTga(ILimage* image, TARGAHEAD *Header) {
  ILubyte Bpp;
  char  ID[255];
  SIO *io = &image->io;
  
  if (SIOread(io, ID, 1, Header->IDLen) != Header->IDLen)
    return IL_FALSE;
  
  /*if (Header->Bpp == 16)
    Bpp = 3;
  else*/
  Bpp = (ILubyte)(Header->Bpp >> 3);
  
  if (!iTexImage(image, Header->Width, Header->Height, 1, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
    return IL_FALSE;
  }
  
  switch (image->Bpp)
  {
    case 1:
      image->Format = IL_COLOUR_INDEX;  // wtf?  How is this possible?
      break;
    case 2:  // 16-bit is not supported directly!
         //image->Format = IL_RGB5_A1;
      /*image->Format = IL_RGBA;
      image->Type = IL_UNSIGNED_SHORT_5_5_5_1_EXT;*/
      //image->Type = IL_UNSIGNED_SHORT_5_6_5_REV;
      
      // Remove?
      //iCloseImage(image);
      //iSetError(IL_FORMAT_NOT_SUPPORTED);
      //return IL_FALSE;
      
      /*image->Bpp = 4;
      image->Format = IL_BGRA;
      image->Type = IL_UNSIGNED_SHORT_1_5_5_5_REV;*/
      
      image->Format = IL_BGR;
      
      break;
    case 3:
      image->Format = IL_BGR;
      break;
    case 4:
      image->Format = IL_BGRA;
      break;
    default:
      iSetError(IL_INVALID_VALUE);
      return IL_FALSE;
  }
  
  
  // @TODO:  Determine this:
  // We assume that no palette is present, but it's possible...
  //  Should we mess with it or not?
  
  
  if (Header->ImageType == TGA_UNMAP_COMP) {
    if (!iUncompressTgaData(image)) {
      return IL_FALSE;
    }
  }
  else {
    if (SIOread(io, image->Data, 1, image->SizeOfData) != image->SizeOfData) {
      return IL_FALSE;
    }
  }
  
  // Go ahead and expand it to 24-bit.
  if (Header->Bpp == 16) {
    if (!i16BitTarga(image))
      return IL_FALSE;
    return IL_TRUE;
  }
  
  return IL_TRUE;
}

static ILboolean iReadBwTga(ILimage* image, TARGAHEAD *Header) {
  char ID[255];
  SIO *io = &image->io;
  
  if (SIOread(io, ID, 1, Header->IDLen) != Header->IDLen)
    return IL_FALSE;
  
  // We assume that no palette is present, but it's possible...
  //  Should we mess with it or not?
  
  if (!iTexImage(image, Header->Width, Header->Height, 1, (ILubyte)(Header->Bpp >> 3), IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL)) {
    return IL_FALSE;
  }
  
  if (Header->ImageType == TGA_BW_COMP) {
    if (!iUncompressTgaData(image)) {
      return IL_FALSE;
    }
  }
  else {
    if (SIOread(io, image->Data, 1, image->SizeOfData) != image->SizeOfData) {
      return IL_FALSE;
    }
  }
  
  return IL_TRUE;
}

static ILboolean iUncompressTgaData(ILimage *image) {
  ILuint  BytesRead = 0, Size, RunLen, i, ToRead;
  ILubyte Header, Color[4];
  ILint c;
  SIO *io = &image->io;
  
  Size = image->Width * image->Height * image->Depth * image->Bpp;
  
  while (BytesRead < Size) {
    Header = (ILubyte)image->io.getchar(image->io.handle);
    if (Header & BIT(7)) {
      Header &= ~BIT(7);
      if (SIOread(io, Color, 1, image->Bpp) != image->Bpp) {
        return IL_FALSE;
      }
      RunLen = (Header+1) * image->Bpp;
      for (i = 0; i < RunLen; i += image->Bpp) {
        // Read the color in, but we check to make sure that we do not go past the end of the image.
        for (c = 0; c < image->Bpp && BytesRead+i+c < Size; c++) {
          image->Data[BytesRead+i+c] = Color[c];
        }
      }
      BytesRead += RunLen;
    }
    else {
      RunLen = (Header+1) * image->Bpp;
      // We have to check that we do not go past the end of the image data.
      if (BytesRead + RunLen > Size)
        ToRead = Size - BytesRead;
      else
        ToRead = RunLen;
      if (SIOread(io, image->Data + BytesRead, 1, ToRead) != ToRead) {
        return IL_FALSE;
      }
      BytesRead += RunLen;

      if (BytesRead + RunLen > Size)
        SIOseek(io, RunLen - ToRead, IL_SEEK_CUR);
    }
  }
  
  return IL_TRUE;
}


// Pretty damn unoptimized
static ILboolean i16BitTarga(ILimage *image) {
  ILushort  *Temp1;
  ILubyte   *Data, *Temp2;
  ILuint    x, PixSize = image->Width * image->Height;
  
  Data = (ILubyte*)ialloc(image->Width * image->Height * 3);
  Temp1 = (ILushort*)image->Data;
  Temp2 = Data;
  
  if (Data == NULL)
    return IL_FALSE;
  
  for (x = 0; x < PixSize; x++) {
    *Temp2++ = (*Temp1 & 0x001F) << 3;  // Blue
    *Temp2++ = (*Temp1 & 0x03E0) >> 2;  // Green
    *Temp2++ = (*Temp1 & 0x7C00) >> 7;  // Red
    
    Temp1++;
  }
  
  if (!iTexImage(image, image->Width, image->Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, Data)) {
    ifree(Data);
    return IL_FALSE;
  }
  
  ifree(Data);
  
  return IL_TRUE;
}


// Internal function used to save the Targa.
// @todo: write header in one read() call
static ILboolean iSaveTargaInternal(ILimage* image)
{
  char  *ID           = iGetString(IL_TGA_ID_STRING);
  char  *AuthName     = iGetString(IL_META_ARTIST);
  char  *AuthComment  = iGetString(IL_META_USER_COMMENT);
  ILboolean Compress;
  ILuint    RleLen;
  ILubyte   *Rle;
  ILenum    Format;
  ILpal   * TempPal = NULL;
  ILimage   *TempImage = NULL;
  ILuint    ExtOffset;
//  char    * Footer = "TRUEVISION-XFILE.";
  char    * idString = iGetString(IL_META_SOFTWARE);
  char    * TempData;
  SIO *     io;

  TARGAHEAD   Header;
  TARGAFOOTER Footer;
  TARGAEXT    Ext;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;

  Compress = iGetInt(IL_TGA_RLE);
  Format = image->Format;

  // build header
  imemclear(&Header, sizeof(Header));
  imemclear(&Footer, sizeof(Footer));
  imemclear(&Ext, sizeof(Ext));

  // clip strings
  if (ID && iCharStrLen(ID) >= 255) ID[255] = 0;
  if (AuthName)     { iCharStrNCpy(Ext.AuthName,     AuthName,     40); ifree(AuthName);     }
  if (AuthComment)  { iCharStrNCpy(Ext.AuthComments, AuthComment, 324); ifree(AuthComment);  }
  
  if (!idString) idString = iGetString(IL_VERSION_NUM);
  if (idString)     { iCharStrNCpy(Ext.SoftwareID,  idString,     40); ifree(idString);   }

  Ext.Size = 495;
  iGetDateTime(&Ext.Month, &Ext.Day, &Ext.Year, &Ext.Hour, &Ext.Minute, &Ext.Second);

  UShort(&Ext.Month);
  UShort(&Ext.Day);
  UShort(&Ext.Year);
  UShort(&Ext.Hour);
  Short(&Ext.Minute);
  UShort(&Ext.Second);

  Ext.SoftwareVer = IL_VERSION;
  UShort(&Ext.SoftwareVer);

  Ext.KeyColor = 3;

  Header.IDLen          = ID ? (ILubyte)iCharStrLen(ID) : 0;
  Header.ColMapPresent  = image->Pal.Palette && image->Pal.PalSize && image->Pal.PalType != IL_PAL_NONE;
  Header.FirstEntry     = 0; // if present, palette follows immediately
  Short   (&Header.FirstEntry);

  switch (image->Format) {
    case IL_COLOUR_INDEX: Header.ImageType = Compress ?  9 : 1; break;
    case IL_BGR:
    case IL_BGRA:         Header.ImageType = Compress ? 10 : 2; break;
    case IL_RGB:
    case IL_RGBA:         Header.ImageType = Compress ? 10 : 2; 
                          iSwapColours(image);
                          break;
    case IL_LUMINANCE:    Header.ImageType = Compress ? 11 : 3; break;

    default:
      // Should convert the types here...
      iSetError(IL_INVALID_VALUE);
      ifree(ID);
      ifree(idString);
      return IL_FALSE;
  }

  switch (image->Pal.PalType) {
    case IL_PAL_NONE:
      Header.ColMapLen = 0;
      Header.ColMapEntSize = 0;
      break;
    case IL_PAL_BGR24:
      Header.ColMapLen = (ILshort)(image->Pal.PalSize / 3);
      Header.ColMapEntSize = 24;
      TempPal = &image->Pal;
      break;
      
    case IL_PAL_RGB24:
    case IL_PAL_RGB32:
    case IL_PAL_RGBA32:
    case IL_PAL_BGR32:
    case IL_PAL_BGRA32:
      TempPal = iConvertPal(&image->Pal, IL_PAL_BGR24);
      if (TempPal == NULL)
        return IL_FALSE;
        Header.ColMapLen = (ILshort)(TempPal->PalSize / 3);
      Header.ColMapEntSize = 24;
      break;
    default:
      iSetError(IL_INVALID_VALUE);
      ifree(ID);
      ifree(idString);
      Header.ColMapLen = 0;
      Header.ColMapEntSize = 0;
      return IL_FALSE;
  }
  Short   (&Header.ColMapLen);
  
  if (image->Bpc > 1) {
    TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
    if (TempImage == NULL) {
      ifree(ID);
      ifree(idString);
      if (Format == IL_RGB || Format == IL_RGBA) {
        iSwapColours(image);
      }
      return IL_FALSE;
    }
  } else {
    TempImage = image;
  }
  
  if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
    TempData = (char*)iGetFlipped(TempImage);
  } else {
    TempData = (char*)TempImage->Data;
  }
  
  Header.OriginX = image->OffX;
  Header.OriginY = image->OffY;
  Header.Width   = image->Width;
  Header.Height  = image->Height;
  Header.Bpp     = image->Bpp << 3;  // Changes to bits per pixel
  Header.ImageDesc = 0;

  Short   (&Header.OriginX);
  Short   (&Header.OriginY);
  UShort  (&Header.Width);
  UShort  (&Header.Height);

  SIOwrite(io, &Header, 1, sizeof(Header));
  SIOwrite(io, ID, sizeof(char), Header.IDLen);
  ifree(ID);

  // Write out the colormap
  if (Header.ColMapPresent) {
    SIOwrite(io, TempPal->Palette, sizeof(ILubyte), TempPal->PalSize);
  }
  
  if (!Compress) {
    SIOwrite(io, TempData, sizeof(ILubyte), TempImage->SizeOfData);
  } else {
    Rle = (ILubyte*)ialloc(TempImage->SizeOfData + TempImage->SizeOfData / 2 + 1);  // max
    if (Rle == NULL) {
      ifree(AuthName);
      ifree(AuthComment);
      ifree(idString);
      return IL_FALSE;
    }
    RleLen = ilRleCompress((unsigned char*)TempData, TempImage->Width, TempImage->Height,
                           TempImage->Depth, TempImage->Bpp, Rle, IL_TGACOMP, NULL);
    
    SIOwrite(io, Rle, 1, RleLen);
    ifree(Rle);
  }
  
  // Write the extension area.
  ExtOffset = SIOtell(io);
  SIOwrite(io, &Ext, 1, sizeof(Ext));
  
  // Write the footer.
  Footer.ExtOff = ExtOffset;
  Footer.DevDirOff = 0; // No developer directory
  iCharStrNCpy(Footer.Signature, "TRUEVISION-XFILE", 16);
  Footer.Reserved = '.';
  Footer.NullChar = 0;

  UInt(&Footer.ExtOff);
  UInt(&Footer.DevDirOff);

  SIOwrite(io, &Footer, 1, sizeof(Footer));
  
  if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
    ifree(TempData);
  }
  if (Format == IL_RGB || Format == IL_RGBA) {
    iSwapColours(image);
  }
  
  if (TempPal != &image->Pal && TempPal != NULL) {
    ifree(TempPal->Palette);
    ifree(TempPal);
  }
  
  if (TempImage != image)
    iCloseImage(TempImage);

  return IL_TRUE;
}

//changed name to iGetDateTime on 20031221 to fix bug 830196
static void iGetDateTime(ILshort *Month, ILshort *Day, ILshort *Yr, ILshort *Hr, ILshort *Min, ILshort *Sec) {
#ifdef DJGPP
  struct date day;
  struct time curtime;
  
  gettime(&curtime);
  getdate(&day);
  
  *Month = day.da_mon;
  *Day = day.da_day;
  *Yr = day.da_year;
  
  *Hr = curtime.ti_hour;
  *Min = curtime.ti_min;
  *Sec = curtime.ti_sec;
  
  return;
#else
  
#ifdef _WIN32
  SYSTEMTIME Time;
  
  GetSystemTime(&Time);
  
  *Month = Time.wMonth;
  *Day = Time.wDay;
  *Yr = Time.wYear;
  
  *Hr = Time.wHour;
  *Min = Time.wMinute;
  *Sec = Time.wSecond;
  
  return;
#else
  
  // FIXME: reentrancy
  time_t now = time(NULL);
  struct tm *tm = gmtime(&now);

  *Month = tm->tm_mon;
  *Day = tm->tm_mday;
  *Yr = tm->tm_year;

  *Hr = tm->tm_hour;
  *Min = tm->tm_min;
  *Sec = tm->tm_sec;

  return;
#endif
#endif
}

ILconst_string iFormatExtsTGA[] = { 
  IL_TEXT("tga"), 
  IL_TEXT("vda"), 
  IL_TEXT("icb"), 
  IL_TEXT("vst"), 
  NULL 
};

ILformat iFormatTGA = { 
  /* .Validate = */ iIsValidTarga, 
  /* .Load     = */ iLoadTargaInternal, 
  /* .Save     = */ iSaveTargaInternal, 
  /* .Exts     = */ iFormatExtsTGA
};

#endif//IL_NO_TGA
