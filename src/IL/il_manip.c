//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/24/2009
//
// Filename: src-IL/src/il_manip.c
//
// Description: Image manipulation
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "il_manip.h"

// Should we add type to the parameter list?
// Copies a 1d block of pixels to the buffer pointed to by Data.
static ILboolean iCopyPixels1D(ILimage *Image, ILuint XOff, ILuint Width, void *Data)
{
  ILuint  x, c, NewBps, NewOff, PixBpp;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      TempData = iGetFlipped(Image);
      if (TempData == NULL)
        return IL_FALSE;
    }
  }

  PixBpp = Image->Bpp * Image->Bpc;

  if (Image->Width < XOff + Width) {
    NewBps = (Image->Width - XOff) * PixBpp;
  }
  else {
    NewBps = Width * PixBpp;
  }
  NewOff = XOff * PixBpp;

  for (x = 0; x < NewBps; x += PixBpp) {
    for (c = 0; c < PixBpp; c++) {
      Temp[x + c] = TempData[(x + NewOff) + c];
    }
  }

  if (TempData != Image->Data)
    ifree(TempData);

  return IL_TRUE;
}


// Copies a 2d block of pixels to the buffer pointed to by Data.
static ILboolean iCopyPixels2D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint Width, ILuint Height, void *Data)
{
  ILuint  x, y, c, NewBps, DataBps, NewXOff, NewHeight, PixBpp;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      TempData = iGetFlipped(Image);
      if (TempData == NULL)
        return IL_FALSE;
    }
  }

  PixBpp = Image->Bpp * Image->Bpc;

  if (Image->Width < XOff + Width)
    NewBps = (Image->Width - XOff) * PixBpp;
  else
    NewBps = Width * PixBpp;

  if (Image->Height < YOff + Height)
    NewHeight = Image->Height - YOff;
  else
    NewHeight = Height;

  DataBps = Width * PixBpp;
  NewXOff = XOff * PixBpp;

  for (y = 0; y < NewHeight; y++) {
    for (x = 0; x < NewBps; x += PixBpp) {
      for (c = 0; c < PixBpp; c++) {
        Temp[y * DataBps + x + c] = 
          TempData[(y + YOff) * Image->Bps + x + NewXOff + c];
      }
    }
  }

  if (TempData != Image->Data)
    ifree(TempData);

  return IL_TRUE;
}


// Copies a 3d block of pixels to the buffer pointed to by Data.
static ILboolean iCopyPixels3D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, void *Data)
{
  ILuint  x, y, z, c, NewBps, DataBps, NewSizePlane, NewH, NewD, NewXOff, PixBpp;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      TempData = iGetFlipped(Image);
      if (TempData == NULL)
        return IL_FALSE;
    }
  }

  PixBpp = Image->Bpp * Image->Bpc;

  if (Image->Width < XOff + Width)
    NewBps = (Image->Width - XOff) * PixBpp;
  else
    NewBps = Width * PixBpp;

  if (Image->Height < YOff + Height)
    NewH = Image->Height - YOff;
  else
    NewH = Height;

  if (Image->Depth < ZOff + Depth)
    NewD = Image->Depth - ZOff;
  else
    NewD = Depth;

  DataBps = Width * PixBpp;
  NewSizePlane = NewBps * NewH;

  NewXOff = XOff * PixBpp;

  for (z = 0; z < NewD; z++) {
    for (y = 0; y < NewH; y++) {
      for (x = 0; x < NewBps; x += PixBpp) {
        for (c = 0; c < PixBpp; c++) {
          Temp[z * NewSizePlane + y * DataBps + x + c] = 
            TempData[(z + ZOff) * Image->SizeOfPlane + (y + YOff) * Image->Bps + x + NewXOff + c];
            //TempData[(z + ZOff) * Image->SizeOfPlane + (y + YOff) * Image->Bps + (x + XOff) * Image->Bpp + c];
        }
      }
    }
  }

  if (TempData != Image->Data)
    ifree(TempData);

  return IL_TRUE;
}


ILuint ILAPIENTRY iCopyPixels(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data) {
  void  *Converted = NULL;
  ILubyte *TempBuff = NULL;
  ILuint  SrcSize, DestSize;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return 0;
  }
  DestSize = Width * Height * Depth * iGetBppFormat(Format) * iGetBpcType(Type);
  if (DestSize == 0) {
    return DestSize;
  }
  if (Data == NULL || Format == IL_COLOUR_INDEX) {
    iSetError(IL_INVALID_PARAM);
    return 0;
  }
  SrcSize = Width * Height * Depth * Image->Bpp * Image->Bpc;

  if (Format == Image->Format && Type == Image->Type) {
    TempBuff = (ILubyte*)Data;
  }
  else {
    TempBuff = (ILubyte*)ialloc(SrcSize);
    if (TempBuff == NULL) {
      return 0;
    }
  }

  if (YOff + Height <= 1) {
    if (!iCopyPixels1D(Image, XOff, Width, TempBuff)) {
      goto failed;
    }
  }
  else if (ZOff + Depth <= 1) {
    if (!iCopyPixels2D(Image, XOff, YOff, Width, Height, TempBuff)) {
      goto failed;
    }
  }
  else {
    if (!iCopyPixels3D(Image, XOff, YOff, ZOff, Width, Height, Depth, TempBuff)) {
      goto failed;
    }
  }

  if (Format == Image->Format && Type == Image->Type) {
    return DestSize;
  }

  Converted = iConvertBuffer(SrcSize, Image->Format, Format, Image->Type, Type, &Image->Pal, TempBuff);
  if (Converted == NULL)
    goto failed;

  memcpy(Data, Converted, DestSize);

  ifree(Converted);
  if (TempBuff != Data)
    ifree(TempBuff);

  return DestSize;

failed:
  if (TempBuff != Data)
    ifree(TempBuff);
  ifree(Converted);
  return 0;
}


static ILboolean iSetPixels1D(ILimage *Image, ILint XOff, ILuint Width, void *Data)
{
  ILuint  c, PixBpp;
  ILint x, NewWidth, SkipX = 0;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      TempData = iGetFlipped(Image);
      if (TempData == NULL)
        return IL_FALSE;
    }
  }

  PixBpp = Image->Bpp * Image->Bpc;

  if (XOff < 0) {
    SkipX = abs(XOff);
    XOff = 0;
  }

  if (Image->Width - Width < (ILuint)XOff) {
    NewWidth = (ILint)Image->Width - XOff;
  }
  else {
    NewWidth = (ILint)Width;
  }

  NewWidth -= SkipX;

  for (x = 0; x < NewWidth; x++) {
    for (c = 0; c < PixBpp; c++) {
      TempData[(ILuint)(x + XOff) * PixBpp + c] = Temp[(ILuint)(x + SkipX) * PixBpp + c];
    }
  }

  if (TempData != Image->Data) {
    ifree(Image->Data);
    Image->Data = TempData;
  }

  return IL_TRUE;
}


static ILboolean iSetPixels2D(ILimage *Image, ILint XOff, ILint YOff, ILuint Width, ILuint Height, void *Data)
{
  ILuint x, y;
  ILuint NewWidth, NewHeight, NewBps, PixBpp, SkipX = 0, SkipY = 0, c;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      TempData = iGetFlipped(Image);
      if (TempData == NULL)
        return IL_FALSE;
    }
  }

  PixBpp = Image->Bpp * Image->Bpc;

  if (XOff < 0) {
    SkipX = (ILuint)abs(XOff);
    XOff = 0;
  }
  if (YOff < 0) {
    SkipY = (ILuint)abs(YOff);
    YOff = 0;
  }

  if (Image->Width < (ILuint)XOff + Width)
    NewWidth = Image->Width - (ILuint)XOff;
  else
    NewWidth = Width;
  NewBps = Width * PixBpp;

  if (Image->Height < (ILuint)YOff + Height)
    NewHeight = Image->Height - (ILuint)YOff;
  else
    NewHeight = Height;

  NewWidth -= SkipX;
  NewHeight -= SkipY;

  for (y = 0; y < NewHeight; y++) {
    for (x = 0; x < NewWidth; x++) {
      for (c = 0; c < PixBpp; c++) {
        TempData[(y + (ILuint)YOff) * Image->Bps + (x + (ILuint)XOff) * PixBpp + c] =
          Temp[(y + SkipY) * NewBps + (x + SkipX) * PixBpp + c];          
      }
    }
  }

  if (TempData != Image->Data) {
    ifree(Image->Data);
    Image->Data = TempData;
  }

  return IL_TRUE;
}


static ILboolean iSetPixels3D(ILimage *Image, ILint XOff, ILint YOff, ILint ZOff, ILuint Width, ILuint Height, ILuint Depth, void *Data)
{
  ILuint  SkipX = 0, SkipY = 0, SkipZ = 0, c, NewBps, NewSizePlane, PixBpp;
  ILuint x, y, z, NewW, NewH, NewD;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      TempData = iGetFlipped(Image);
      if (TempData == NULL)
        return IL_FALSE;
    }
  }

  PixBpp = Image->Bpp * Image->Bpc;

  if (XOff < 0) {
    SkipX = (ILuint)abs(XOff);
    XOff = 0;
  }
  if (YOff < 0) {
    SkipY = (ILuint)abs(YOff);
    YOff = 0;
  }
  if (ZOff < 0) {
    SkipZ = (ILuint)abs(ZOff);
    ZOff = 0;
  }

  if (Image->Width < (ILuint)XOff + Width)
    NewW = Image->Width - (ILuint)XOff;
  else
    NewW = Width;
  NewBps = Width * PixBpp;

  if (Image->Height < (ILuint)YOff + Height)
    NewH = Image->Height - (ILuint)YOff;
  else
    NewH = Height;

  if (Image->Depth < (ILuint)ZOff + Depth)
    NewD = Image->Depth - (ILuint)ZOff;
  else
    NewD = Depth;
  NewSizePlane = NewBps * Height;

  NewW -= SkipX;
  NewH -= SkipY;
  NewD -= SkipZ;

  for (z = 0; z < NewD; z++) {
    for (y = 0; y < NewH; y++) {
      for (x = 0; x < NewW; x++) {
        for (c = 0; c < PixBpp; c++) {
          TempData[(z + (ILuint)ZOff) * Image->SizeOfPlane + (y + (ILuint)YOff) * Image->Bps + (x + (ILuint)XOff) * PixBpp + c] =
            Temp[(z + SkipZ) * NewSizePlane + (y + SkipY) * NewBps + (x + SkipX) * PixBpp + c];
        }
      }
    }
  }

  if (TempData != Image->Data) {
    ifree(Image->Data);
    Image->Data = TempData;
  }

  return IL_TRUE;
}

void iSetPixels(ILimage *Image, ILint XOff, ILint YOff, ILint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data)
{
  void *Converted;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return;
  }

  if (Data == NULL) {
    iSetError(IL_INVALID_PARAM);
    return;
  }

  if (Format == Image->Format && Type == Image->Type) {
    Converted = (void*)Data;
  }
  else {
    Converted = iConvertBuffer(Width * Height * Depth * iGetBppFormat(Format) * iGetBpcType(Type), Format, Image->Format, Type, Image->Type, NULL, Data);
    if (!Converted)
      return;
  }

  if ((ILuint)YOff + Height <= 1) {
    iSetPixels1D(Image, XOff, Width, Converted);
  }
  else if ((ILuint)ZOff + Depth <= 1) {
    iSetPixels2D(Image, XOff, YOff, Width, Height, Converted);
  }
  else {
    iSetPixels3D(Image, XOff, YOff, ZOff, Width, Height, Depth, Converted);
  }

  if (Format == Image->Format && Type == Image->Type) {
    return;
  }

  if (Converted != Data)
    ifree(Converted);
}

//  Ripped from Platinum (Denton's sources)
//  This could very well easily be changed to a 128x128 image instead...needed?

//! Creates an ugly 64x64 black and yellow checkerboard image.
ILboolean iDefaultImage(ILimage *Image)
{
  ILubyte *TempData;
  ILubyte Yellow[3] = { 18, 246, 243 };
  ILubyte Black[3]  = { 0, 0, 0 };
  ILubyte *ColorPtr = Yellow;  // The start color
  ILboolean Color = IL_TRUE;

  // Loop Variables
  ILint v, w, x, y;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (!iTexImage(Image, 64, 64, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL)) {
    return IL_FALSE;
  }
  Image->Origin = IL_ORIGIN_LOWER_LEFT;
  TempData = Image->Data;

  for (v = 0; v < 8; v++) {
    // We do this because after a "block" line ends, the next row of blocks
    // above starts with the ending colour, but the very inner loop switches them.
    if (Color) {
      Color = IL_FALSE;
      ColorPtr = Black;
    }
    else {
      Color = IL_TRUE;
      ColorPtr = Yellow;
    }

    for (w = 0; w < 8; w++) {
      for (x = 0; x < 8; x++) {
        for (y = 0; y < 8; y++, TempData += Image->Bpp) {
          TempData[0] = ColorPtr[0];
          TempData[1] = ColorPtr[1];
          TempData[2] = ColorPtr[2];
        }

        // Switch to alternate between black and yellow
        if (Color) {
          Color = IL_FALSE;
          ColorPtr = Black;
        }
        else {
          Color = IL_TRUE;
          ColorPtr = Yellow;
        }
      }
    }
  }

  return IL_TRUE;
}

ILubyte* iGetAlpha(ILimage *Image, ILenum Type) {
  ILimage   *TempImage;
  ILubyte   *Alpha;
  ILuint    i, j, Bpc, Size, AlphaOff;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }

  Bpc = iGetBpcType(Type);
  if (Bpc == 0) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }

  if (Image->Type == Type) {
    TempImage = Image;
  } else {
    TempImage = iConvertImage(Image, Image->Format, Type);
    if (TempImage == NULL)
      return NULL;
  }

  Size = Image->Width * Image->Height * Image->Depth * TempImage->Bpp;
  Alpha = (ILubyte*)ialloc(Size / TempImage->Bpp * Bpc);
  if (Alpha == NULL) {
    if (TempImage != Image)
      iCloseImage(TempImage);
    return NULL;
  }

  // If format has no alpha, fill with opaque.
  if (!iFormatHasAlpha(TempImage->Format)) {
    memset(Alpha, 0xFF, Size / TempImage->Bpp * Bpc);
    if (TempImage != Image)
      iCloseImage(TempImage);
    return Alpha;
  }

  // If our format is alpha, just return a copy.
  if (TempImage->Format == IL_ALPHA) {
    memcpy(Alpha, TempImage->Data, TempImage->SizeOfData);
    return Alpha;
  }
    
  if (TempImage->Format == IL_LUMINANCE_ALPHA)
    AlphaOff = 2;
  else
    AlphaOff = 4;

  for (i = AlphaOff-1, j = 0; i < Size; i += AlphaOff, j++)
    memcpy(Alpha + j*Bpc, TempImage->Data + i * Bpc, Bpc);

  if (TempImage != Image)
    iCloseImage(TempImage);

  return Alpha;
}

// sets the Alpha value to a specific value for each pixel in the image
ILboolean iSetAlpha(ILimage *Image, ILdouble AlphaValue)
{
  ILboolean ret = IL_TRUE;
  ILuint    i,Size;
  ILuint    AlphaOff = 0;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  AlphaValue = IL_CLAMP(AlphaValue);

  switch (Image->Format)
  {
    case IL_RGB:
      ret = iConvertImages(Image, IL_RGBA, Image->Type);
    case IL_RGBA:
      AlphaOff = 4;
    break;
    case IL_BGR:
      ret = iConvertImages(Image, IL_BGRA, Image->Type);
    case IL_BGRA:
      AlphaOff = 4;
      break;
    case IL_LUMINANCE:
      ret = iConvertImages(Image, IL_LUMINANCE_ALPHA, Image->Type);
    case IL_LUMINANCE_ALPHA:
      AlphaOff = 2;
      break;
    case IL_ALPHA:
      AlphaOff = 1;
      break;
    case IL_COLOUR_INDEX: //@TODO use palette with alpha
      ret = iConvertImages(Image, IL_RGBA, Image->Type);
      AlphaOff = 4;
      break;
  }
  if (ret == IL_FALSE) {
    // Error has been set by ilConvertImages.
    return IL_FALSE;
  }
  Size = Image->Width * Image->Height * Image->Depth * Image->Bpp;

  switch (Image->Type)
  {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE: {
      const ILubyte alpha = (ILubyte)(AlphaValue * IL_MAX_UNSIGNED_BYTE + .5);
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        iGetImageDataUByte(Image)[i] = alpha;
      break;
    }
    case IL_SHORT:
    case IL_UNSIGNED_SHORT: {
      const ILushort alpha = (ILushort)(AlphaValue * IL_MAX_UNSIGNED_SHORT + .5);
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        iGetImageDataUShort(Image)[i] = alpha;
      break;
    }
    case IL_INT:
    case IL_UNSIGNED_INT: {
      const ILushort alpha = (ILushort)(AlphaValue * IL_MAX_UNSIGNED_INT + .5);
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        iGetImageDataUInt(Image)[i] = alpha;
      break;
    }
    case IL_FLOAT: {
      const ILfloat alpha = (ILfloat)AlphaValue;
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        iGetImageDataFloat(Image)[i] = alpha;
      break;
    }
    case IL_DOUBLE: {
      const ILdouble alpha  = AlphaValue;
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        iGetImageDataDouble(Image)[i] = alpha;
      break;
    }
  }

  return IL_TRUE;
}

ILboolean iClampNTSC(ILimage *Image) {
  ILuint x, y, z, c;
  ILuint Offset = 0;

  if (Image == NULL) {
      iSetError(IL_ILLEGAL_OPERATION);
      return IL_FALSE;
  }

  // BP: TODO: should clip to corresponding values instead
  // BP: TODO: palettes?

  if (Image->Type != IL_UNSIGNED_BYTE)  // Should we set an error here?
    return IL_FALSE;

  for (z = 0; z < Image->Depth; z++) {
    for (y = 0; y < Image->Height; y++) {
      for (x = 0; x < Image->Width; x++) {
        for (c = 0; c < Image->Bpp; c++) {
          Image->Data[Offset + c] = IL_LIMIT(Image->Data[Offset + c], 16, 235);
        }
        Offset += Image->Bpp;
      }
    }
  }

  return IL_TRUE;
}
