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


ILAPI void ILAPIENTRY iFlipBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num)
{
  ILubyte *StartPtr, *EndPtr;
  ILuint y, d;
  const ILuint size = line_num * line_size;

  for (d = 0; d < depth; d++) {
    StartPtr = buff + d * size;
    EndPtr   = buff + d * size + size;

    for (y = 0; y < (line_num/2); y++) {
      EndPtr -= line_size; 
      iMemSwap(StartPtr, EndPtr, line_size);
      StartPtr += line_size;
    }
  }
}

// Just created for internal use.
static ILubyte* iFlipNewBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num)
{
  ILubyte *data;
  ILubyte *s1, *s2;
  ILuint y, d;
  const ILuint size = line_num * line_size;

  if ((data = (ILubyte*)ialloc(depth*size)) == NULL)
    return IL_FALSE;

  for (d = 0; d < depth; d++) {
    s1 = buff + d * size;
    s2 = data + d * size+size;

    for (y = 0; y < line_num; y++) {
      s2 -= line_size; 
      memcpy(s2,s1,line_size);
      s1 += line_size;
    }
  }
  return data;
}


// Flips an image over its x axis
ILboolean ILAPIENTRY iFlipImage(ILimage *Image)
{
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image->Origin = (Image->Origin == IL_ORIGIN_LOWER_LEFT) ?
            IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;

  iFlipBuffer(Image->Data, Image->Depth, Image->Bps, Image->Height);

  return IL_TRUE;
}

// Just created for internal use.
ILubyte* ILAPIENTRY iGetFlipped(ILimage *img)
{
  if (img == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }
  return iFlipNewBuffer(img->Data,img->Depth,img->Bps,img->Height);
}


//@JASON New routine created 28/03/2001
//! Mirrors an image over its y axis
ILboolean ILAPIENTRY iMirrorImage(ILimage *Image) {
  ILubyte   *Data, *DataPtr, *Temp;
  ILuint    y, d, PixLine;
  ILint   x, c;
  ILushort  *ShortPtr, *TempShort;
  ILuint    *IntPtr, *TempInt;
  ILdouble  *DblPtr, *TempDbl;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Data = (ILubyte*)ialloc(Image->SizeOfData);
  if (Data == NULL)
    return IL_FALSE;

  PixLine = Image->Bps / Image->Bpc;
  switch (Image->Bpc)
  {
    case 1:
      Temp = Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        DataPtr = Data + d * Image->SizeOfPlane;
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, Temp++) {
              DataPtr[y * PixLine + x * Image->Bpp + c] = *Temp;
            }
          }
        }
      }
      break;

    case 2:
      TempShort = (ILushort*)Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        ShortPtr = (ILushort*)(Data + d * Image->SizeOfPlane);
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, TempShort++) {
              ShortPtr[y * PixLine + x * Image->Bpp + c] = *TempShort;
            }
          }
        }
      }
      break;

    case 4:
      TempInt = (ILuint*)Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        IntPtr = (ILuint*)(Data + d * Image->SizeOfPlane);
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, TempInt++) {
              IntPtr[y * PixLine + x * Image->Bpp + c] = *TempInt;
            }
          }
        }
      }
      break;

    case 8:
      TempDbl = (ILdouble*)Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        DblPtr = (ILdouble*)(Data + d * Image->SizeOfPlane);
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, TempDbl++) {
              DblPtr[y * PixLine + x * Image->Bpp + c] = *TempDbl;
            }
          }
        }
      }
      break;
  }

  ifree(Image->Data);
  Image->Data = Data;

  return IL_TRUE;
}


// Should we add type to the parameter list?
// Copies a 1d block of pixels to the buffer pointed to by Data.
ILboolean iCopyPixels1D(ILimage *Image, ILuint XOff, ILuint Width, void *Data)
{
  ILuint  x, c, NewBps, NewOff, PixBpp;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != Image->Origin) {
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
ILboolean iCopyPixels2D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint Width, ILuint Height, void *Data)
{
  ILuint  x, y, c, NewBps, DataBps, NewXOff, NewHeight, PixBpp;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != Image->Origin) {
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
ILboolean iCopyPixels3D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, void *Data)
{
  ILuint  x, y, z, c, NewBps, DataBps, NewSizePlane, NewH, NewD, NewXOff, PixBpp;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != Image->Origin) {
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


ILboolean iSetPixels1D(ILimage *Image, ILint XOff, ILuint Width, void *Data)
{
  ILuint  c, SkipX = 0, PixBpp;
  ILint x, NewWidth;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != Image->Origin) {
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

  if (Image->Width < XOff + Width) {
    NewWidth = Image->Width - XOff;
  }
  else {
    NewWidth = Width;
  }

  NewWidth -= SkipX;

  for (x = 0; x < NewWidth; x++) {
    for (c = 0; c < PixBpp; c++) {
      TempData[(x + XOff) * PixBpp + c] = Temp[(x + SkipX) * PixBpp + c];
    }
  }

  if (TempData != Image->Data) {
    ifree(Image->Data);
    Image->Data = TempData;
  }

  return IL_TRUE;
}


ILboolean iSetPixels2D(ILimage *Image, ILint XOff, ILint YOff, ILuint Width, ILuint Height, void *Data)
{
  ILuint  c, SkipX = 0, SkipY = 0, NewBps, PixBpp;
  ILint x, y, NewWidth, NewHeight;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != Image->Origin) {
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
  if (YOff < 0) {
    SkipY = abs(YOff);
    YOff = 0;
  }

  if (Image->Width < XOff + Width)
    NewWidth = Image->Width - XOff;
  else
    NewWidth = Width;
  NewBps = Width * PixBpp;

  if (Image->Height < YOff + Height)
    NewHeight = Image->Height - YOff;
  else
    NewHeight = Height;

  NewWidth -= SkipX;
  NewHeight -= SkipY;

  for (y = 0; y < NewHeight; y++) {
    for (x = 0; x < NewWidth; x++) {
      for (c = 0; c < PixBpp; c++) {
        TempData[(y + YOff) * Image->Bps + (x + XOff) * PixBpp + c] =
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


ILboolean iSetPixels3D(ILimage *Image, ILint XOff, ILint YOff, ILint ZOff, ILuint Width, ILuint Height, ILuint Depth, void *Data)
{
  ILuint  SkipX = 0, SkipY = 0, SkipZ = 0, c, NewBps, NewSizePlane, PixBpp;
  ILint x, y, z, NewW, NewH, NewD;
  ILubyte *Temp = (ILubyte*)Data, *TempData = Image->Data;

  if (ilIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != Image->Origin) {
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
  if (YOff < 0) {
    SkipY = abs(YOff);
    YOff = 0;
  }
  if (ZOff < 0) {
    SkipZ = abs(ZOff);
    ZOff = 0;
  }

  if (Image->Width < XOff + Width)
    NewW = Image->Width - XOff;
  else
    NewW = Width;
  NewBps = Width * PixBpp;

  if (Image->Height < YOff + Height)
    NewH = Image->Height - YOff;
  else
    NewH = Height;

  if (Image->Depth < ZOff + Depth)
    NewD = Image->Depth - ZOff;
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
          TempData[(z + ZOff) * Image->SizeOfPlane + (y + YOff) * Image->Bps + (x + XOff) * PixBpp + c] =
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

  if (YOff + Height <= 1) {
    iSetPixels1D(Image, XOff, Width, Converted);
  }
  else if (ZOff + Depth <= 1) {
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
  ILushort  *AlphaShort;
  ILuint    *AlphaInt;
  ILdouble  *AlphaDbl;
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

  switch (TempImage->Format)
  {
    case IL_RGB:
    case IL_BGR:
    case IL_LUMINANCE:
    case IL_COLOUR_INDEX:  // @TODO: Make IL_COLOUR_INDEX separate.
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

  switch (TempImage->Type)
  {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      for (i = AlphaOff-1, j = 0; i < Size; i += AlphaOff, j++)
        Alpha[j] = TempImage->Data[i];
      break;

    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      AlphaShort = (ILushort*)Alpha;
      for (i = AlphaOff-1, j = 0; i < Size; i += AlphaOff, j++)
        AlphaShort[j] = ((ILushort*)TempImage->Data)[i];
      break;

    case IL_INT:
    case IL_UNSIGNED_INT:
    case IL_FLOAT:  // Can throw float in here, because it's the same size.
      AlphaInt = (ILuint*)Alpha;
      for (i = AlphaOff-1, j = 0; i < Size; i += AlphaOff, j++)
        AlphaInt[j] = ((ILuint*)TempImage->Data)[i];
      break;

    case IL_DOUBLE:
      AlphaDbl = (ILdouble*)Alpha;
      for (i = AlphaOff-1, j = 0; i < Size; i += AlphaOff, j++)
        AlphaDbl[j] = ((ILdouble*)TempImage->Data)[i];
      break;
  }

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
      const ILbyte alpha = (ILubyte)(AlphaValue * IL_MAX_UNSIGNED_BYTE + .5);
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        Image->Data[i] = alpha;
      break;
    }
    case IL_SHORT:
    case IL_UNSIGNED_SHORT: {
      const ILushort alpha = (ILushort)(AlphaValue * IL_MAX_UNSIGNED_SHORT + .5);
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        ((ILushort*)Image->Data)[i] = alpha;
      break;
    }
    case IL_INT:
    case IL_UNSIGNED_INT: {
      const ILushort alpha = (ILushort)(AlphaValue * IL_MAX_UNSIGNED_INT + .5);
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        ((ILuint*)Image->Data)[i] = alpha;
      break;
    }
    case IL_FLOAT: {
      const ILfloat alpha = (ILfloat)AlphaValue;
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        ((ILfloat*)Image->Data)[i] = alpha;
      break;
    }
    case IL_DOUBLE: {
      const ILdouble alpha  = AlphaValue;
      for (i = AlphaOff-1; i < Size; i += AlphaOff)
        ((ILdouble*)Image->Data)[i] = alpha;
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
