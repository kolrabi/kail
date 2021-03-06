//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/09/2009
//
// Filename: src-IL/src/il_dds-save.c
//
// Description: Saves a DirectDraw Surface (.dds) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_DDS

#include "il_dds.h"
#include "il_manip.h"
#include "il_internal.h"
#include "il_stack.h"
#include "il_states.h"

#include <limits.h>

static ILuint     Compress(ILimage *Image, ILenum DXTCFormat);
static ILboolean  WriteHeader(ILimage *Image, ILenum DXTCFormat, ILuint CubeFlags);
static ILboolean  Get3DcBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos, ILubyte channel);

static void       ChooseAlphaEndpoints(ILubyte *Block, ILubyte *a0, ILubyte *a1);
static void       GenAlphaBitMask(ILubyte a0, ILubyte a1, ILubyte *In, ILubyte *Mask, ILubyte *Out);
static ILboolean  GetAlphaBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos);

static void       ChooseEndpoints(ILushort *Block, ILushort *ex0, ILushort *ex1);
static ILuint     GenBitMask(ILushort ex0, ILushort ex1, ILuint NumCols, ILushort *In, ILubyte *Alpha, Color888 *OutCol);
static ILboolean  GetBlock(ILushort *Block, ILushort *Data, ILimage *Image, ILuint XPos, ILuint YPos);

static void       CorrectEndDXT1(ILushort *ex0, ILushort *ex1, ILboolean HasAlpha);

// static void       ShortToColor565(ILushort Pixel, Color565 *Colour);
static void       ShortToColor888(ILushort Pixel, Color888 *Colour);
// static ILushort   Color888ToShort(Color888 *Colour);

static ILuint     Distance(Color888 *c1, Color888 *c2);

//! Checks if an image is a cubemap
static ILuint GetCubemapInfo(ILimage* image, ILuint* faces)
{
  ILint indices[] = { -1, -1, -1,  -1, -1, -1 };
  ILimage *img;
  ILuint  ret = 0, i;
  ILint   srcMipmapCount = 0, srcImagesCount = 0, mipmapCount;

  if (image == NULL)
    return 0;

  iGetiv(image, IL_NUM_IMAGES, &srcImagesCount, 1);
  if (srcImagesCount != 5) //write only complete cubemaps (TODO?)
    return 0;

  img = image;
  iGetiv(image, IL_NUM_MIPMAPS, &srcMipmapCount, 1);
  mipmapCount = srcMipmapCount;

  for (i = 0; i < 6; ++i) {
    switch (img->CubeFlags)
    {
      case DDS_CUBEMAP_POSITIVEX:
        indices[i] = 0;
        break;
      case DDS_CUBEMAP_NEGATIVEX:
        indices[i] = 1;
        break;
      case DDS_CUBEMAP_POSITIVEY:
        indices[i] = 2;
        break;
      case DDS_CUBEMAP_NEGATIVEY:
        indices[i] = 3;
        break;
      case DDS_CUBEMAP_POSITIVEZ:
        indices[i] = 4;
        break;
      case DDS_CUBEMAP_NEGATIVEZ:
        indices[i] = 5;
        break;
    }
    iGetiv(image, IL_NUM_MIPMAPS, &srcMipmapCount, 1);
    if (srcMipmapCount != mipmapCount)
      return 0; //equal # of mipmaps required

    ret |= img->CubeFlags;
    img = img->Next;
  }

  for (i = 0; i < 6; ++i)
    if (indices[i] == -1)
      return 0; //one face not found

  if (ret != 0) //should always be true
    ret |= DDS_CUBEMAP;

  for (i = 0; i < 6; ++i)
    faces[indices[i]] = i;

  return ret;
}


// Internal function used to save the Dds.
ILboolean iSaveDdsInternal(ILimage *Image)
{
  ILenum  DXTCFormat;
  ILuint counter, numMipMaps, numFaces, i;
  ILubyte *CurData = NULL;
  ILuint CubeTable[6] = { 0 };
  ILuint  CubeFlags;
  ILimage *SubImage;

  CubeFlags = GetCubemapInfo(Image, CubeTable);

  DXTCFormat = (ILenum)iGetInt(IL_DXTC_FORMAT);
  WriteHeader(Image, DXTCFormat, CubeFlags);

  if (CubeFlags != 0) {
    //numFaces = iGetIntegerImage(Image, IL_NUM_FACES); // Should always be 5 for now
    iGetiv(Image, IL_NUM_FACES, (ILint*)&numFaces, 1);
  } else {
    numFaces = 0;
  }

  // numMipMaps = iGetIntegerImage(Image, IL_NUM_MIPMAPS); //this assumes all faces have same # of mipmaps
  iGetiv(Image, IL_NUM_MIPMAPS, (ILint*)&numMipMaps, 1);

  for (i = 0; i <= numFaces; ++i) {
    for (counter = 0; counter <= numMipMaps; counter++) {
      SubImage = iGetSubImage(Image, CubeTable[i]);
      SubImage = iGetMipmap(SubImage, counter);

      if (!SubImage) {
        iSetError(IL_INTERNAL_ERROR);
        return IL_FALSE;
      }

      if (SubImage->Origin != IL_ORIGIN_UPPER_LEFT) {
        CurData = SubImage->Data;
        SubImage->Data = iGetFlipped(SubImage);
        if (SubImage->Data == NULL) {
          SubImage->Data = CurData;
          return IL_FALSE;
        }
      }

      if (!Compress(SubImage, DXTCFormat))
        return IL_FALSE;

      if (SubImage->Origin != IL_ORIGIN_UPPER_LEFT) {
        ifree(SubImage->Data);
        SubImage->Data = CurData;
      }
    }

  }

  return IL_TRUE;
}


// @TODO:  Finish this, as it is incomplete.
ILboolean WriteHeader(ILimage *Image, ILenum DXTCFormat, ILuint CubeFlags)
{
  ILuint i, FourCC, Flags1 = 0, Flags2 = 0, ddsCaps1 = 0,
          LinearSize, BlockSize, ddsCaps2 = 0;
  ILint numMipMaps;
  SIO *io = &Image->io;

  Flags1 |= DDS_MIPMAPCOUNT | DDS_WIDTH | DDS_HEIGHT | DDS_CAPS | DDS_PIXELFORMAT;
  if (DXTCFormat != IL_DXT_NO_COMP)
    Flags2 |= DDS_FOURCC;

  if (Image->Depth > 1)
    Flags1 |= DDS_DEPTH;

  // @TODO:  Fix the pre-multiplied alpha problem.
  /*if (DXTCFormat == IL_DXT2)
    DXTCFormat = IL_DXT3;
  else if (DXTCFormat == IL_DXT4)
    DXTCFormat = IL_DXT5;*/

  switch (DXTCFormat)
  {
    case IL_DXT_NO_COMP:
      FourCC = 0;
      break;    
    case IL_DXT1:
    case IL_DXT1A:
      FourCC = IL_MAKEFOURCC('D','X','T','1');
      break;
    case IL_DXT2:
      FourCC = IL_MAKEFOURCC('D','X','T','2');
      break;
    case IL_DXT3:
      FourCC = IL_MAKEFOURCC('D','X','T','3');
      break;
    case IL_DXT4:
      FourCC = IL_MAKEFOURCC('D','X','T','4');
      break;
    case IL_DXT5:
      FourCC = IL_MAKEFOURCC('D','X','T','5');
      break;
    case IL_ATI1N:
      FourCC = IL_MAKEFOURCC('A', 'T', 'I', '1');
      break;
    case IL_3DC:
      FourCC = IL_MAKEFOURCC('A','T','I','2');
      break;
    case IL_RXGB:
      FourCC = IL_MAKEFOURCC('R','X','G','B');
      break;
    default:
      // Error!
      iSetError(IL_INTERNAL_ERROR);  // Should never happen, though.
      return IL_FALSE;
  }

  if (DXTCFormat == IL_DXT_NO_COMP) {
    LinearSize = ( Image->Width * 8 * (iFormatHasAlpha(Image->Format)?4:3) + 7 ) / 8;
    Flags1 |= DDS_PITCH;
  } else {
    Flags1 |= DDS_LINEARSIZE;
    if (DXTCFormat == IL_DXT1 || DXTCFormat == IL_DXT1A || DXTCFormat == IL_ATI1N) {
      BlockSize = 8;
    }
    else {
      BlockSize = 16;
    }
    LinearSize = (((Image->Width + 3)/4) * ((Image->Height + 3)/4)) * BlockSize * Image->Depth;
  }


  SIOwrite(io, "DDS ", 1, 4);
  SaveLittleUInt(io,124);   // Size1
  SaveLittleUInt(io,Flags1);    // Flags1
  SaveLittleUInt(io,Image->Height);
  SaveLittleUInt(io,Image->Width);
  SaveLittleUInt(io,LinearSize);  // LinearSize

  if (Image->Depth > 1) {
    SaveLittleUInt(io,Image->Depth);      // Depth
    ddsCaps2 |= DDS_VOLUME;
  } else {
    SaveLittleUInt(io,0);           // Depth
  }

  //numMipMaps = iGetIntegerImage(Image, IL_NUM_MIPMAPS);
  iGetiv(Image, IL_NUM_MIPMAPS, &numMipMaps, 1);
  SaveLittleInt(io, numMipMaps + 1);  // MipMapCount

  for (i = 0; i < 11; i++)
    SaveLittleUInt(io,0);   // Not used

  SaveLittleUInt(io,32);      // Size2
  SaveLittleUInt(io,Flags2);    // Flags2
  SaveLittleUInt(io,FourCC);    // FourCC

  if (DXTCFormat == IL_DXT_NO_COMP) {
    SaveLittleUInt(io,iFormatHasAlpha(Image->Format) ? 32 :24);              // RGBBitCount
    SaveLittleUInt(io,iGetMaskFormat(IL_BGRA, 0));      // RBitMask
    SaveLittleUInt(io,iGetMaskFormat(IL_BGRA, 1));      // GBitMask
    SaveLittleUInt(io,iGetMaskFormat(IL_BGRA, 2));      // BBitMask
    SaveLittleUInt(io,iFormatHasAlpha(Image->Format) ? iGetMaskFormat(IL_BGRA, 3) : 0);      // RGBAlphaBitMask
  } else {
    SaveLittleUInt(io,0);     // RGBBitCount
    SaveLittleUInt(io,0);     // RBitMask
    SaveLittleUInt(io,0);     // GBitMask
    SaveLittleUInt(io,0);     // BBitMask
    SaveLittleUInt(io,0);     // RGBAlphaBitMask
  }

  ddsCaps1 |= DDS_TEXTURE;
  //changed 20040516: set mipmap flag on mipmap images
  //(non-compressed .dds files still not supported,
  //though)
  if (iGetInteger(Image, IL_NUM_MIPMAPS) > 0)
    ddsCaps1 |= DDS_MIPMAP | DDS_COMPLEX;
  if (CubeFlags != 0) {
    ddsCaps1 |= DDS_COMPLEX;
    ddsCaps2 |= CubeFlags;
  }

  SaveLittleUInt(io,ddsCaps1);  // ddsCaps1
  SaveLittleUInt(io,ddsCaps2);  // ddsCaps2
  SaveLittleUInt(io,0);     // ddsCaps3
  SaveLittleUInt(io,0);     // ddsCaps4
  SaveLittleUInt(io,0);     // TextureStage

  return IL_TRUE;
}

#endif//IL_NO_DDS


ILuint ILAPIENTRY iGetDXTCData(ILimage *Image, void *Buffer, ILuint BufferSize, ILenum DXTCFormat) {
  ILubyte *CurData = NULL;
  ILuint  retVal;
  ILuint BlockNum;

  if (Buffer == NULL) {  // Return the number that will be written with a subsequent call.
    BlockNum = ((Image->Width + 3)/4) * ((Image->Height + 3)/4) * Image->Depth;

    switch (DXTCFormat)
    {
      case IL_DXT1:
      case IL_DXT1A:
      case IL_ATI1N:
        return BlockNum * 8;
      case IL_DXT2:
      case IL_DXT3:
      case IL_DXT4:
      case IL_DXT5:
      case IL_3DC:
      case IL_RXGB:
        return BlockNum * 16;
      default:
        iSetError(IL_FORMAT_NOT_SUPPORTED);
        return 0;
    }
  }

  if (DXTCFormat == Image->DxtcFormat && Image->DxtcSize && Image->DxtcData) {
    memcpy(Buffer, Image->DxtcData, IL_MIN(BufferSize, Image->DxtcSize));
    return IL_MIN(BufferSize, Image->DxtcSize);
  }

  if (Image->Origin != IL_ORIGIN_UPPER_LEFT) {
    CurData = Image->Data;
    Image->Data = iGetFlipped(Image);
    if (Image->Data == NULL) {
      Image->Data = CurData;
      return 0;
    }
  }

  //@TODO: Is this the best way to do this?
  iSetOutputLump(Image, Buffer, BufferSize);
  retVal = Compress(Image, DXTCFormat);

  if (Image->Origin != IL_ORIGIN_UPPER_LEFT) {
    ifree(Image->Data);
    Image->Data = CurData;
  }

  return retVal;
}


// Added the next two functions based on Charles Bloom's rant at
//  http://cbloomrants.blogspot.com/2008/12/12-08-08-dxtc-summary.html.
//  This code is by ryg and from the Molly Rocket forums:
//  https://mollyrocket.com/forums/viewtopic.php?t=392.
static ILint Mul8Bit(ILint a, ILint b)
{
  ILint t = a*b + 128;
  return (t + (t >> 8)) >> 8;
}

static ILushort As16Bit(ILint r, ILint g, ILint b)
{
  return (ILushort)((Mul8Bit(r,31) << 11) + (Mul8Bit(g,63) << 5) + Mul8Bit(b,31));
}

static ILushort *CompressTo565(ILimage *Image)
{
  ILimage   *TempImage;
  ILushort  *Data;
  ILuint    i, j;

  if ((Image->Type != IL_UNSIGNED_BYTE && Image->Type != IL_BYTE) || Image->Format == IL_COLOUR_INDEX) {
    TempImage = iConvertImage(Image, IL_BGRA, IL_UNSIGNED_BYTE);  // @TODO: Needs to be BGRA.
    if (TempImage == NULL)
      return NULL;
  }
  else {
    TempImage = Image;
  }

  Data = (ILushort*)ialloc(Image->Width * Image->Height * 2 * Image->Depth);
  if (Data == NULL) {
    if (TempImage != Image)
      iCloseImage(TempImage);
    return NULL;
  }

  //changed 20040623: Use TempImages format :)
  switch (TempImage->Format)
  {
    case IL_RGB:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j++) {
        /*Data[j]  = (TempImage->Data[i  ] >> 3) << 11;
        Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
        Data[j] |=  TempImage->Data[i+2] >> 3;*/
        Data[j] = As16Bit(TempImage->Data[i], TempImage->Data[i+1], TempImage->Data[i+2]);
      }
      break;

    case IL_RGBA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j++) {
        /*Data[j]  = (TempImage->Data[i  ] >> 3) << 11;
        Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
        Data[j] |=  TempImage->Data[i+2] >> 3;*/
        Data[j] = As16Bit(TempImage->Data[i], TempImage->Data[i+1], TempImage->Data[i+2]);
      }
      break;

    case IL_BGR:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j++) {
        /*Data[j]  = (TempImage->Data[i+2] >> 3) << 11;
        Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
        Data[j] |=  TempImage->Data[i  ] >> 3;*/
        Data[j] = As16Bit(TempImage->Data[i+2], TempImage->Data[i+1], TempImage->Data[i]);
      }
      break;

    case IL_BGRA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j++) {
        /*Data[j]  = (TempImage->Data[i+2] >> 3) << 11;
        Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
        Data[j] |=  TempImage->Data[i  ] >> 3;*/
        Data[j] = As16Bit(TempImage->Data[i+2], TempImage->Data[i+1], TempImage->Data[i]);
      }
      break;

    case IL_LUMINANCE:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i++, j++) {
        //@TODO: Do better conversion here.
        /*Data[j]  = (TempImage->Data[i] >> 3) << 11;
        Data[j] |= (TempImage->Data[i] >> 2) << 5;
        Data[j] |=  TempImage->Data[i] >> 3;*/
        Data[j] = As16Bit(TempImage->Data[i], TempImage->Data[i], TempImage->Data[i]);
      }
      break;

    case IL_LUMINANCE_ALPHA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 2, j++) {
        //@TODO: Do better conversion here.
        /*Data[j]  = (TempImage->Data[i] >> 3) << 11;
        Data[j] |= (TempImage->Data[i] >> 2) << 5;
        Data[j] |=  TempImage->Data[i] >> 3;*/
        Data[j] = As16Bit(TempImage->Data[i], TempImage->Data[i], TempImage->Data[i]);
      }
      break;

    case IL_ALPHA:
      memset(Data, 0, Image->Width * Image->Height * 2 * Image->Depth);
      break;
  }

  if (TempImage != Image)
    iCloseImage(TempImage);

  return Data;
}

static ILushort *CompressTo565PremulAlpha(ILimage *Image)
{
  ILimage   *TempImage;
  ILushort  *Data;
  ILuint    i, j;
  ILubyte   Alpha;

  if ((Image->Type != IL_UNSIGNED_BYTE && Image->Type != IL_BYTE) || Image->Format == IL_COLOUR_INDEX) {
    TempImage = iConvertImage(Image, IL_BGRA, IL_UNSIGNED_BYTE);  // @TODO: Needs to be BGRA.
    if (TempImage == NULL)
      return NULL;
  }
  else {
    TempImage = Image;
  }

  Data = (ILushort*)ialloc(Image->Width * Image->Height * 2 * Image->Depth);
  if (Data == NULL) {
    if (TempImage != Image)
      iCloseImage(TempImage);
    return NULL;
  }

  //changed 20040623: Use TempImages format :)
  switch (TempImage->Format)
  {
    case IL_RGB:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j++) {
        Data[j] = As16Bit( TempImage->Data[i], TempImage->Data[i+1], TempImage->Data[i+2]);
      }
      break;

    case IL_RGBA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j++) {
        Alpha = TempImage->Data[i+3];
        Data[j] = As16Bit( (Alpha*TempImage->Data[i]) >> 8, (Alpha*TempImage->Data[i+1]) >> 8, (Alpha*TempImage->Data[i+2])>>8);
      }
      break;

    case IL_BGR:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j++) {
        Data[j] = As16Bit(TempImage->Data[i+2], TempImage->Data[i+1], TempImage->Data[i]);
      }
      break;

    case IL_BGRA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j++) {
        Alpha = TempImage->Data[i+3];
        Data[j] = As16Bit( (Alpha*TempImage->Data[i+2]) >> 8, (Alpha*TempImage->Data[i+1]) >> 8, (Alpha*TempImage->Data[i])>>8);
      }
      break;

    case IL_LUMINANCE:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i++, j++) {
        Data[j] = As16Bit(TempImage->Data[i], TempImage->Data[i], TempImage->Data[i]);
      }
      break;

    case IL_LUMINANCE_ALPHA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 2, j++) {
        Alpha = TempImage->Data[i+1];
        Data[j] = As16Bit((Alpha*TempImage->Data[i])>>8, (Alpha*TempImage->Data[i])>>8, (Alpha*TempImage->Data[i])>>8);
      }
      break;

    case IL_ALPHA:
      memset(Data, 0, Image->Width * Image->Height * 2 * Image->Depth);
      break;
  }

  if (TempImage != Image)
    iCloseImage(TempImage);

  return Data;
}

static ILubyte *CompressTo88(ILimage *Image)
{
  ILimage   *TempImage;
  ILubyte   *Data;
  ILuint    i, j;

  if ((Image->Type != IL_UNSIGNED_BYTE && Image->Type != IL_BYTE) || Image->Format == IL_COLOUR_INDEX) {
    TempImage = iConvertImage(Image, IL_BGR, IL_UNSIGNED_BYTE);  // @TODO: Needs to be BGRA.
    if (TempImage == NULL)
      return NULL;
  }
  else {
    TempImage = Image;
  }

  Data = (ILubyte*)ialloc(Image->Width * Image->Height * 2 * Image->Depth);
  if (Data == NULL) {
    if (TempImage != Image)
      iCloseImage(TempImage);
    return NULL;
  }

  //changed 20040623: Use TempImage's format :)
  switch (TempImage->Format)
  {
    case IL_RGB:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j += 2) {
        Data[j  ] = TempImage->Data[i+1];
        Data[j+1] = TempImage->Data[i  ];
      }
      break;

    case IL_RGBA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j += 2) {
        Data[j  ] = TempImage->Data[i+1];
        Data[j+1] = TempImage->Data[i  ];
      }
      break;

    case IL_BGR:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j += 2) {
        Data[j  ] = TempImage->Data[i+1];
        Data[j+1] = TempImage->Data[i+2];
      }
      break;

    case IL_BGRA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j += 2) {
        Data[j  ] = TempImage->Data[i+1];
        Data[j+1] = TempImage->Data[i+2];
      }
      break;

    case IL_LUMINANCE:
    case IL_LUMINANCE_ALPHA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i++, j += 2) {
        Data[j  ] = Data[j+1] = 0; //??? Luminance is no normal map format...
      }
      break;
  }

  if (TempImage != Image)
    iCloseImage(TempImage);

  return Data;
}

static void CompressToRXGB(ILimage *Image, ILushort** xgb, ILubyte** r)
{
  ILimage   *TempImage;
  ILuint    i, j;
  ILushort  *Data;
  ILubyte   *Alpha;

  *xgb = NULL;
  *r = NULL;

  if ((Image->Type != IL_UNSIGNED_BYTE && Image->Type != IL_BYTE) || Image->Format == IL_COLOUR_INDEX) {
    TempImage = iConvertImage(Image, IL_BGR, IL_UNSIGNED_BYTE);  // @TODO: Needs to be BGRA.
    if (TempImage == NULL)
      return;
  }
  else {
    TempImage = Image;
  }

  *xgb = (ILushort*)ialloc(Image->Width * Image->Height * 2 * Image->Depth);
  *r = (ILubyte*)ialloc(Image->Width * Image->Height * Image->Depth);
  if (*xgb == NULL || *r == NULL) {
    if (TempImage != Image)
      iCloseImage(TempImage);
    return;
  }

  //Alias pointers to be able to use copy'n'pasted code :)
  Data = *xgb;
  Alpha = *r;

  switch (TempImage->Format)
  {
    case IL_RGB:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j++) {
        Alpha[j] = (ILubyte) ( TempImage->Data[i]             );
        Data[j]  = (ILushort)((TempImage->Data[i+1] >> 2) << 5);
        Data[j] |= (ILushort)( TempImage->Data[i+2] >> 3      );
      }
      break;

    case IL_RGBA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j++) {
        Alpha[j]  = (ILubyte) ( TempImage->Data[i]);
        Data[j]   = (ILushort)((TempImage->Data[i+1] >> 2) << 5);
        Data[j]  |= (ILushort)( TempImage->Data[i+2] >> 3);
      }
      break;

    case IL_BGR:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 3, j++) {
        Alpha[j]  = (ILubyte) ( TempImage->Data[i+2]);
        Data[j]   = (ILushort)((TempImage->Data[i+1] >> 2) << 5);
        Data[j]  |= (ILushort)( TempImage->Data[i  ] >> 3);
      }
      break;

    case IL_BGRA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 4, j++) {
        Alpha[j]  = (ILubyte) ( TempImage->Data[i+2]);
        Data[j]   = (ILushort)((TempImage->Data[i+1] >> 2) << 5);
        Data[j]  |= (ILushort)( TempImage->Data[i  ] >> 3);
      }
      break;

    case IL_LUMINANCE:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i++, j++) {
        Alpha[j]  = (ILubyte) ( TempImage->Data[i]);
        Data[j]   = (ILushort)((TempImage->Data[i] >> 2) << 5);
        Data[j]  |= (ILushort)( TempImage->Data[i] >> 3);
      }
      break;

    case IL_LUMINANCE_ALPHA:
      for (i = 0, j = 0; i < TempImage->SizeOfData; i += 2, j++) {
        Alpha[j]  = (ILubyte) ( TempImage->Data[i]);
        Data[j]   = (ILushort)((TempImage->Data[i] >> 2) << 5);
        Data[j]  |= (ILushort)( TempImage->Data[i] >> 3);
      }
      break;
  }

  if (TempImage != Image)
    iCloseImage(TempImage);
}


static ILuint Compress(ILimage *Image, ILenum DXTCFormat)
{
  ILushort  *Data, Block[16], ex0, ex1, *Runner16, t0, t1;
  ILuint    x, y, z, i, BitMask;//, Rms1, Rms2;
  ILubyte   *Alpha, AlphaBlock[16], AlphaBitMask[6], /*AlphaOut[16],*/ a0, a1;
  ILboolean HasAlpha;
  ILuint    Count = 0;
  ILubyte   *Data3Dc, *Runner8;
  SIO *     io = &Image->io;

  if (DXTCFormat == IL_3DC) {
    Data3Dc = CompressTo88(Image);
    if (Data3Dc == NULL) {
      iTrace("**** CompressTo88 returned NULL");
      return 0;
    }

    Runner8 = Data3Dc;

    for (z = 0; z < Image->Depth; z++) {
      for (y = 0; y < Image->Height; y += 4) {
        for (x = 0; x < Image->Width; x += 4) {
          Get3DcBlock(AlphaBlock, Runner8, Image, x, y, 0);
          ChooseAlphaEndpoints(AlphaBlock, &a0, &a1);
          GenAlphaBitMask(a0, a1, AlphaBlock, AlphaBitMask, NULL);
          SIOputc(io, a0);
          SIOputc(io, a1);
          SIOwrite(io, AlphaBitMask, 1, 6);

          Get3DcBlock(AlphaBlock, Runner8, Image, x, y, 1);
          ChooseAlphaEndpoints(AlphaBlock, &a0, &a1);
          GenAlphaBitMask(a0, a1, AlphaBlock, AlphaBitMask, NULL);
          SIOputc(io, a0);
          SIOputc(io, a1);
          SIOwrite(io, AlphaBitMask, 1, 6);

          Count += 16;
        }
      }
      Runner8 += Image->Width * Image->Height * 2;
    }
    ifree(Data3Dc);
  }

  else if (DXTCFormat == IL_ATI1N)
  {
    ILimage *TempImage;

    if (Image->Bpp != 1) {
      TempImage = iConvertImage(Image, IL_LUMINANCE, IL_UNSIGNED_BYTE);
      if (TempImage == NULL) {
        iTrace("**** Could not convert to grayscale");
        return 0;
      }
    }
    else {
      TempImage = Image;
    }

    Runner8 = TempImage->Data;

    for (z = 0; z < Image->Depth; z++) {
      for (y = 0; y < Image->Height; y += 4) {
        for (x = 0; x < Image->Width; x += 4) {
          GetAlphaBlock(AlphaBlock, Runner8, Image, x, y);
          ChooseAlphaEndpoints(AlphaBlock, &a0, &a1);
          GenAlphaBitMask(a0, a1, AlphaBlock, AlphaBitMask, NULL);
          SIOputc(io, a0);
          SIOputc(io, a1);
          SIOwrite(io, AlphaBitMask, 1, 6);
          Count += 8;
        }
      }
      Runner8 += Image->Width * Image->Height;
    }

    if (TempImage != Image)
      iCloseImage(TempImage);
  }
  else
  {
    // libsquish generates better quality output than DevIL does, so we try it next.
#ifdef IL_USE_DXTC_SQUISH
    ILubyte  *ByteData, *BlockData;
    ILuint DXTCSize;
    
    if (iGetInt(IL_SQUISH_COMPRESS) && Image->Depth == 1) {  // See if we need to use the squish library.
      if (DXTCFormat == IL_DXT1 || DXTCFormat == IL_DXT1A || DXTCFormat == IL_DXT3 || DXTCFormat == IL_DXT5) {
        // libsquish needs data as RGBA 32-bit.
        if (Image->Format != IL_RGBA || Image->Type != IL_UNSIGNED_BYTE) {  // No need to convert if already this format/type.
          ByteData = iConvertBuffer(Image->SizeOfData, Image->Format, IL_RGBA, Image->Type, IL_UNSIGNED_BYTE, NULL, Image->Data);
          if (ByteData == NULL) {
            iTrace("**** Could not convert to RGBA unsigned byte buffer");
            return 0;
          }
        }
        else
          ByteData = Image->Data;

        // Get compressed data here.
        BlockData = iSquishCompressDXT(ByteData, Image->Width, Image->Height, 1, DXTCFormat, &DXTCSize);
        if (BlockData == NULL) {
          iTrace("**** iSquishCompressDXT returned NULL, falling back to internal");
          goto nosquish;
        }

        if ((ILuint)SIOwrite(io, BlockData, 1, DXTCSize) != DXTCSize) {
          if (ByteData != Image->Data)
            ifree(ByteData);
          ifree(BlockData);
          return 0;
        }

        if (ByteData != Image->Data)
          ifree(ByteData);
        ifree(BlockData);

        return Image->Width * Image->Height * 4;  // Either compresses all or none.
      }
    }
nosquish:
#endif//IL_USE_DXTC_SQUISH

    if (DXTCFormat == IL_DXT_NO_COMP) {
      Data = NULL;
      Alpha = iConvertBuffer(Image->SizeOfData, Image->Format, iFormatHasAlpha(Image->Format) ? IL_BGRA : IL_BGR, Image->Type, IL_UNSIGNED_BYTE, NULL, Image->Data);
    } else if (DXTCFormat == IL_RXGB) {
      CompressToRXGB(Image, &Data, &Alpha);
      if (Data == NULL || Alpha == NULL) {
        iTrace("**** CompressToRXGB returned NULL");
        if (Data != NULL)
          ifree(Data);
        if (Alpha != NULL)
          ifree(Alpha);
        return 0;
      }
    } else  {
      Alpha = iGetAlpha(Image, IL_UNSIGNED_BYTE);
      if (Alpha == NULL) {
        iTrace("**** iGetAlpha returned NULL");
        return 0;
      }

      if (DXTCFormat == IL_DXT2 || DXTCFormat == IL_DXT4) 
      {
        Data = CompressTo565PremulAlpha(Image);
      }
      else
      {
        Data = CompressTo565(Image);
      }

      if (Data == NULL) {
        iTrace("**** CompressTo565 returned NULL");
        ifree(Alpha);
        return 0;
      }
    }

    Runner8 = Alpha;
    Runner16 = Data;

    switch (DXTCFormat)
    {
      case IL_DXT_NO_COMP:
        Count = Image->Width*Image->Height*Image->Depth * (iFormatHasAlpha(Image->Format) ? 4 : 3);
        SIOwrite(io, Runner8, Count, 1);
        break;

      case IL_DXT1:
      case IL_DXT1A:
        for (z = 0; z < Image->Depth; z++) {
          for (y = 0; y < Image->Height; y += 4) {
            for (x = 0; x < Image->Width; x += 4) {
              GetAlphaBlock(AlphaBlock, Runner8, Image, x, y);
              HasAlpha = IL_FALSE;
              for (i = 0 ; i < 16; i++) {
                if (AlphaBlock[i] < 128) {
                  HasAlpha = IL_TRUE;
                  break;
                }
              }

              GetBlock(Block, Runner16, Image, x, y);
              ChooseEndpoints(Block, &ex0, &ex1);
              CorrectEndDXT1(&ex0, &ex1, HasAlpha);
              SaveLittleUShort(io, ex0);
              SaveLittleUShort(io, ex1);
              if (HasAlpha)
                BitMask = GenBitMask(ex0, ex1, 3, Block, AlphaBlock, NULL);
              else
                BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
              SaveLittleUInt(io,BitMask);
              Count += 8;
            }
          }

          Runner16 += Image->Width * Image->Height;
          Runner8 += Image->Width * Image->Height;
        }
        break;

      case IL_DXT2:
      case IL_DXT3:
        for (z = 0; z < Image->Depth; z++) {
          for (y = 0; y < Image->Height; y += 4) {
            for (x = 0; x < Image->Width; x += 4) {
              GetAlphaBlock(AlphaBlock, Runner8, Image, x, y);
              for (i = 0; i < 16; i += 2) {
                SIOputc(io, (ILubyte)(((AlphaBlock[i+1] >> 4) << 4) | (AlphaBlock[i] >> 4)));
              }

              GetBlock(Block, Runner16, Image, x, y);
              ChooseEndpoints(Block, &t0, &t1);
              ex0 = IL_MAX(t0, t1);
              ex1 = IL_MIN(t0, t1);
              CorrectEndDXT1(&ex0, &ex1, 0);
              SaveLittleUShort(io, ex0);
              SaveLittleUShort(io, ex1);
              BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
              SaveLittleUInt(io,BitMask);
              Count += 16;
            }
          }

          Runner16 += Image->Width * Image->Height;
          Runner8 += Image->Width * Image->Height;
        }
        break;

      case IL_RXGB:
      case IL_DXT4:
      case IL_DXT5:
        for (z = 0; z < Image->Depth; z++) {
          for (y = 0; y < Image->Height; y += 4) {
            for (x = 0; x < Image->Width; x += 4) {
              GetAlphaBlock(AlphaBlock, Runner8, Image, x, y);
              ChooseAlphaEndpoints(AlphaBlock, &a0, &a1);
              GenAlphaBitMask(a0, a1, AlphaBlock, AlphaBitMask, NULL/*AlphaOut*/);
              SIOputc(io, a0);
              SIOputc(io, a1);
              SIOwrite(io, AlphaBitMask, 1, 6);

              GetBlock(Block, Runner16, Image, x, y);
              ChooseEndpoints(Block, &t0, &t1);
              ex0 = IL_MAX(t0, t1);
              ex1 = IL_MIN(t0, t1);
              CorrectEndDXT1(&ex0, &ex1, 0);
              SaveLittleUShort(io, ex0);
              SaveLittleUShort(io, ex1);
              BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
              SaveLittleUInt(io,BitMask);
              Count += 16;
            }
          }

          Runner16 += Image->Width * Image->Height;
          Runner8 += Image->Width * Image->Height;
        }
        break;

        default:
          iTrace("**** Unsupported DXT compression format: 0x%04x", DXTCFormat);

    }

    ifree(Data);
    ifree(Alpha);
  } //else no 3DC

  return Count;  // Returns 0 if no compression was done.
}


// Assumed to be 16-bit (5:6:5).
ILboolean GetBlock(ILushort *Block, ILushort *Data, ILimage *Image, ILuint XPos, ILuint YPos)
{
    ILuint x, y, i = 0, Offset = YPos * Image->Width + XPos;

  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
        if (XPos + x < Image->Width && YPos + y < Image->Height)
        Block[i++] = Data[Offset + x];
      else
        // Variant of bugfix from https://sourceforge.net/forum/message.php?msg_id=5486779.
        //  If we are out of bounds of the image, just copy the adjacent data.
          Block[i++] = Data[Offset];
    }
    // We do not want to read past the end of the image.
    if (YPos + y + 1 < Image->Height)
      Offset += Image->Width;
  }

  return IL_TRUE;
}


ILboolean GetAlphaBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos)
{
  ILuint x, y, i = 0, Offset = YPos * Image->Width + XPos;

  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
        if (XPos + x < Image->Width && YPos + y < Image->Height)
        Block[i++] = Data[Offset + x];
      else
        // Variant of bugfix from https://sourceforge.net/forum/message.php?msg_id=5486779.
        //  If we are out of bounds of the image, just copy the adjacent data.
          Block[i++] = Data[Offset];
    }
    // We do not want to read past the end of the image.
    if (YPos + y + 1 < Image->Height)
      Offset += Image->Width;
  }

  return IL_TRUE;
}

ILboolean Get3DcBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos, ILubyte channel)
{
  ILuint x, y, i = 0;
  ILuint Offset = 2*(YPos * Image->Width + XPos) + channel;

  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      if (x < Image->Width && y < Image->Height)
        Block[i++] = Data[Offset + 2*x];
      else
        Block[i++] = Data[Offset];
    }
        Offset += 2*Image->Width;
  }

  return IL_TRUE;
}

/*
void ShortToColor565(ILushort Pixel, Color565 *Colour)
{
  Colour->nRed   = (Pixel & 0xF800) >> 11;
  Colour->nGreen = (Pixel & 0x07E0) >> 5;
  Colour->nBlue  = (Pixel & 0x001F);
  return;
}
*/

void ShortToColor888(ILushort Pixel, Color888 *Colour)
{
  Colour->r = (ILubyte)(((Pixel & 0xF800) >> 11) << 3);
  Colour->g = (ILubyte)(((Pixel & 0x07E0) >> 5)  << 2);
  Colour->b = (ILubyte)(((Pixel & 0x001F))       << 3);
  return;
}

/*ILushort Color888ToShort(Color888 *Colour)
{
  return (ILushort)(((Colour->r >> 3) << 11) | ((Colour->g >> 2) << 5) | (Colour->b >> 3));
}*/


ILuint GenBitMask(ILushort ex0, ILushort ex1, ILuint NumCols, ILushort *In, ILubyte *Alpha, Color888 *OutCol)
{
  ILuint    i, j, Closest, Dist, BitMask = 0;
  ILubyte   Mask[16];
  Color888  c, Colours[4];

  ShortToColor888(ex0, &Colours[0]);
  ShortToColor888(ex1, &Colours[1]);
  if (NumCols == 3) {
    Colours[2].r = (Colours[0].r + Colours[1].r) / 2;
    Colours[2].g = (Colours[0].g + Colours[1].g) / 2;
    Colours[2].b = (Colours[0].b + Colours[1].b) / 2;
    Colours[3].r = (Colours[0].r + Colours[1].r) / 2;
    Colours[3].g = (Colours[0].g + Colours[1].g) / 2;
    Colours[3].b = (Colours[0].b + Colours[1].b) / 2;
  }
  else {  // NumCols == 4
    Colours[2].r = (2 * Colours[0].r + Colours[1].r + 1) / 3;
    Colours[2].g = (2 * Colours[0].g + Colours[1].g + 1) / 3;
    Colours[2].b = (2 * Colours[0].b + Colours[1].b + 1) / 3;
    Colours[3].r = (Colours[0].r + 2 * Colours[1].r + 1) / 3;
    Colours[3].g = (Colours[0].g + 2 * Colours[1].g + 1) / 3;
    Colours[3].b = (Colours[0].b + 2 * Colours[1].b + 1) / 3;
  }

  for (i = 0; i < 16; i++) {
    if (Alpha) {  // Test to see if we have 1-bit transparency
      if (Alpha[i] < 128) {
        Mask[i] = 3;  // Transparent
        if (OutCol) {
          OutCol[i].r = Colours[3].r;
          OutCol[i].g = Colours[3].g;
          OutCol[i].b = Colours[3].b;
        }
        continue;
      }
    }

    // If no transparency, try to find which colour is the closest.
    Closest = UINT_MAX;
    ShortToColor888(In[i], &c);
    for (j = 0; j < NumCols; j++) {
      Dist = Distance(&c, &Colours[j]);
      if (Dist < Closest) {
        Closest = Dist;
        Mask[i] = (ILubyte)j;
        if (OutCol) {
          OutCol[i].r = Colours[j].r;
          OutCol[i].g = Colours[j].g;
          OutCol[i].b = Colours[j].b;
        }
      }
    }
  }

  for (i = 0; i < 16; i++) {
    BitMask |= (ILuint)(Mask[i] << (i*2));
  }

  return BitMask;
}


void GenAlphaBitMask(ILubyte a0, ILubyte a1, ILubyte *In, ILubyte *Mask, ILubyte *Out)
{
  ILubyte Alphas[8], M[16];
  ILuint  i, j, Closest, Dist;

  Alphas[0] = a0;
  Alphas[1] = a1;

  // 8-alpha or 6-alpha block?
  if (a0 > a1) {
    // 8-alpha block:  derive the other six alphas.
    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
    Alphas[2] = (6 * Alphas[0] + 1 * Alphas[1] + 3) / 7;  // bit code 010
    Alphas[3] = (5 * Alphas[0] + 2 * Alphas[1] + 3) / 7;  // bit code 011
    Alphas[4] = (4 * Alphas[0] + 3 * Alphas[1] + 3) / 7;  // bit code 100
    Alphas[5] = (3 * Alphas[0] + 4 * Alphas[1] + 3) / 7;  // bit code 101
    Alphas[6] = (2 * Alphas[0] + 5 * Alphas[1] + 3) / 7;  // bit code 110
    Alphas[7] = (1 * Alphas[0] + 6 * Alphas[1] + 3) / 7;  // bit code 111
  }
  else {
    // 6-alpha block.
    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
    Alphas[2] = (4 * Alphas[0] + 1 * Alphas[1] + 2) / 5;  // Bit code 010
    Alphas[3] = (3 * Alphas[0] + 2 * Alphas[1] + 2) / 5;  // Bit code 011
    Alphas[4] = (2 * Alphas[0] + 3 * Alphas[1] + 2) / 5;  // Bit code 100
    Alphas[5] = (1 * Alphas[0] + 4 * Alphas[1] + 2) / 5;  // Bit code 101
    Alphas[6] = 0x00;                   // Bit code 110
    Alphas[7] = 0xFF;                   // Bit code 111
  }

  for (i = 0; i < 16; i++) {
    Closest = UINT_MAX;
    for (j = 0; j < 8; j++) {
      Dist = (ILuint)abs((ILint)In[i] - (ILint)Alphas[j]);
      if (Dist < Closest) {
        Closest = Dist;
        M[i] = (ILubyte)j;
      }
    }
  }

  if (Out) {
    for (i = 0; i < 16; i++) {
      Out[i] = Alphas[M[i]];
    }
  }

  //this was changed 20040623. There was a shift bug in here. Now the code
  //produces much higher quality images.
  // First three bytes.
  Mask[0] = (ILubyte)((M[0]) | (M[1] << 3) | ((M[2] & 0x03) << 6));
  Mask[1] = (ILubyte)(((M[2] & 0x04) >> 2) | (M[3] << 1) | (M[4] << 4) | ((M[5] & 0x01) << 7));
  Mask[2] = (ILubyte)(((M[5] & 0x06) >> 1) | (M[6] << 2) | (M[7] << 5));

  // Second three bytes.
  Mask[3] = (ILubyte)((M[8]) | (M[9] << 3) | ((M[10] & 0x03) << 6));
  Mask[4] = (ILubyte)(((M[10] & 0x04) >> 2) | (M[11] << 1) | (M[12] << 4) | ((M[13] & 0x01) << 7));
  Mask[5] = (ILubyte)(((M[13] & 0x06) >> 1) | (M[14] << 2) | (M[15] << 5));

  return;
}


ILuint Distance(Color888 *c1, Color888 *c2)
{
  return  (c1->r - c2->r) * (c1->r - c2->r) +
      (c1->g - c2->g) * (c1->g - c2->g) +
      (c1->b - c2->b) * (c1->b - c2->b);
}

//#define Sum(c) ((c)->r + (c)->g + (c)->b)
#define NormSquared(c) ((c)->r * (c)->r + (c)->g * (c)->g + (c)->b * (c)->b)

void ChooseEndpoints(ILushort *Block, ILushort *ex0, ILushort *ex1)
{
  ILuint    i;
  Color888  Colours[16];
  ILuint   Lowest=0, Highest=0;

  for (i = 0; i < 16; i++) {
    ShortToColor888(Block[i], &Colours[i]);
  
    if (NormSquared(&Colours[i]) < NormSquared(&Colours[Lowest]))
      Lowest = i;
    if (NormSquared(&Colours[i]) > NormSquared(&Colours[Highest]))
      Highest = i;
  }
  *ex0 = Block[Highest];
  *ex1 = Block[Lowest];
}

#undef Sum
#undef NormSquared


void ChooseAlphaEndpoints(ILubyte *Block, ILubyte *a0, ILubyte *a1)
{
  ILuint  i, Lowest = 0xFF, Highest = 0;

  for (i = 0; i < 16; i++) {
    if (Block[i] < Lowest)
      Lowest = Block[i];
    if (Block[i] > Highest)
      Highest = Block[i];
  }

  *a0 = (ILubyte)Lowest;
  *a1 = (ILubyte)Highest;
}


void CorrectEndDXT1(ILushort *ex0, ILushort *ex1, ILboolean HasAlpha)
{
  ILushort Temp;

  if (HasAlpha) {
    if (*ex0 > *ex1) {
      Temp = *ex0;
      *ex0 = *ex1;
      *ex1 = Temp;
    }
  }
  else {
    if (*ex0 < *ex1) {
      Temp = *ex0;
      *ex0 = *ex1;
      *ex1 = Temp;
    }
  }

  return;
}

// TODO: doesnt really belong here
ILubyte* iCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DXTCFormat, ILuint *DXTCSize) {
  ILimage *TempImage;
  ILuint  BuffSize;
  ILubyte *Buffer;

  if ((DXTCFormat != IL_DXT1 && DXTCFormat != IL_DXT1A && DXTCFormat != IL_DXT2 && DXTCFormat != IL_DXT3 && DXTCFormat != IL_DXT4 && DXTCFormat != IL_DXT5)
    || Data == NULL || Width == 0 || Height == 0 || Depth == 0) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }

  // libsquish generates better quality output than DevIL does, so we try it next.
#ifdef IL_USE_DXTC_SQUISH
  if (ilIsEnabled(IL_SQUISH_COMPRESS) && Depth == 1) {  // See if we need to use the nVidia Texture Tools library.
    if (DXTCFormat == IL_DXT1 || DXTCFormat == IL_DXT1A || DXTCFormat == IL_DXT3 || DXTCFormat == IL_DXT5) {
      // libsquish needs data as RGBA 32-bit.
      // Get compressed data here.
      return iSquishCompressDXT(Data, Width, Height, 1, DXTCFormat, DXTCSize);
    }
  }
#endif//IL_USE_DXTC_SQUISH

  // neither NVTT nor SQUISH used, create temporary image, use internal dxt compression

  TempImage = (ILimage*)ialloc(sizeof(ILimage));
  memset(TempImage, 0, sizeof(ILimage));
  TempImage->Width        = Width;
  TempImage->Height       = Height;
  TempImage->Depth        = Depth;
  TempImage->Bpp          = 4;  // RGBA or BGRA
  TempImage->Format       = IL_BGRA;
  TempImage->Bpc          = 1;  // Unsigned bytes only
  TempImage->Type         = IL_UNSIGNED_BYTE;
  TempImage->SizeOfPlane  = TempImage->Bps * Height;
  TempImage->SizeOfData   = TempImage->SizeOfPlane * Depth;
  TempImage->Origin       = IL_ORIGIN_UPPER_LEFT;
  TempImage->Data         = Data;

  BuffSize = iGetDXTCData(TempImage, NULL, 0, DXTCFormat);
  if (BuffSize == 0) {
    TempImage->Data = NULL;
    iCloseImage(TempImage);
    return NULL;
  }

  Buffer = (ILubyte*)ialloc(BuffSize);
  if (Buffer == NULL) {
    TempImage->Data = NULL;
    iCloseImage(TempImage);
    return NULL;
  }

  if (iGetDXTCData(TempImage, Buffer, BuffSize, DXTCFormat) != BuffSize) {
    ifree(Buffer);
    TempImage->Data = NULL;
    iCloseImage(TempImage);
    return NULL;
  }
  *DXTCSize = BuffSize;

  TempImage->Data = NULL;
  iCloseImage(TempImage);

  return Buffer;
}
