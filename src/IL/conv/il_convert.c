//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_convert.c
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_manip.h"
#include "il_states.h"

#include <limits.h>


ILimage *iConvertPalette(ILimage *Image, ILenum DestFormat)
{
  static const ILfloat LumFactor[3] = { 0.212671f, 0.715160f, 0.072169f };  // http://www.inforamp.net/~poynton/ and libpng's libpng.txt - Used for conversion to luminance.
  ILimage   *NewImage = NULL; //, *CurImage = NULL;
  ILuint    i, j, k, c, Size;
  ILubyte   LumBpp = 1;
  ILfloat   Resultf;
  ILubyte   *Temp = NULL;
  ILboolean Converted;
  ILboolean HasAlpha = IL_FALSE;

  NewImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
  if (NewImage == NULL) {
    return NULL;
  }

  iCopyImageAttr(NewImage, Image);

  if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE || Image->Bpp != 1) {
    iCloseImage(NewImage);
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }

  if (DestFormat == IL_LUMINANCE || DestFormat == IL_LUMINANCE_ALPHA) {
    if (NewImage->Pal.Palette)
      ifree(NewImage->Pal.Palette);
    if (DestFormat == IL_LUMINANCE_ALPHA)
      LumBpp = 2;

    switch (Image->Pal.PalType)
    {
      case IL_PAL_RGB24:
      case IL_PAL_RGB32:
      case IL_PAL_RGBA32:
        Temp = (ILubyte*)ialloc(LumBpp * Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType));
        if (Temp == NULL)
          goto alloc_error;

        Size = iGetBppPal(Image->Pal.PalType);
        for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
          Resultf = 0.0f;
          for (c = 0; c < Size; c++) {
            Resultf += Image->Pal.Palette[i + c] * LumFactor[c];
          }
          Temp[k] = (ILubyte)Resultf;
          if (LumBpp == 2) {
            if (Image->Pal.PalType == IL_PAL_RGBA32)
              Temp[k+1] = Image->Pal.Palette[i + 3];
            else
              Temp[k+1] = 0xff;
          }
        }

        break;

      case IL_PAL_BGR24:
      case IL_PAL_BGR32:
      case IL_PAL_BGRA32:
        Temp = (ILubyte*)ialloc(LumBpp * Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType));
        if (Temp == NULL)
          goto alloc_error;

        Size = iGetBppPal(Image->Pal.PalType);
        for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
          Resultf = 0.0f;  j = 2;
          for (c = 0; c < Size; c++, j--) {
            Resultf += Image->Pal.Palette[i + c] * LumFactor[j];
          }
          Temp[k] = (ILubyte)Resultf;
          if (LumBpp == 2) {
            if (Image->Pal.PalType == IL_PAL_BGRA32)
              Temp[k+1] = Image->Pal.Palette[i + 3];
            else
              Temp[k+1] = 0xff;
          }
        }

        break;
    }

    NewImage->Pal.Palette = NULL;
    NewImage->Pal.PalSize = 0;
    NewImage->Pal.PalType = IL_PAL_NONE;
    NewImage->Format = DestFormat;
    NewImage->Bpp = LumBpp;
    NewImage->Bps = NewImage->Width * LumBpp;
    NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
    NewImage->Data = (ILubyte*)ialloc(NewImage->SizeOfData);
    if (NewImage->Data == NULL)
      goto alloc_error;

    if (LumBpp == 2) {
      for (i = 0; i < Image->SizeOfData; i++) {
        NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
        NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
      }
    }
    else {
      for (i = 0; i < Image->SizeOfData; i++) {
        NewImage->Data[i] = Temp[Image->Data[i]];
      }
    }

    ifree(Temp);

    return NewImage;
  }
  else if (DestFormat == IL_ALPHA) {
    if (NewImage->Pal.Palette)
      ifree(NewImage->Pal.Palette);

    switch (Image->Pal.PalType)
    {
      // Opaque, so all the values are 0xFF.
      case IL_PAL_RGB24:
      case IL_PAL_RGB32:
      case IL_PAL_BGR24:
      case IL_PAL_BGR32:
        HasAlpha = IL_FALSE;
        break;

      case IL_PAL_BGRA32:
      case IL_PAL_RGBA32:
        HasAlpha = IL_TRUE;
        Temp = (ILubyte*)ialloc(1 * Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType));
        if (Temp == NULL)
          goto alloc_error;

        Size = iGetBppPal(Image->Pal.PalType);
        for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += 1) {
          Temp[k] = Image->Pal.Palette[i + 3];
        }

        break;
    }

    NewImage->Pal.Palette = NULL;
    NewImage->Pal.PalSize = 0;
    NewImage->Pal.PalType = IL_PAL_NONE;
    NewImage->Format = DestFormat;
    NewImage->Bpp = LumBpp;
    NewImage->Bps = NewImage->Width * 1;  // Alpha is only one byte.
    NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
    NewImage->Data = (ILubyte*)ialloc(NewImage->SizeOfData);
    if (NewImage->Data == NULL)
      goto alloc_error;

    if (HasAlpha) {
      for (i = 0; i < Image->SizeOfData; i++) {
        NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
        NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
      }
    }
    else {  // No alpha, opaque.
      for (i = 0; i < Image->SizeOfData; i++) {
        NewImage->Data[i] = 0xFF;
      }
    }

    ifree(Temp);

    return NewImage;
  }

  //NewImage->Format = iGetPalBaseType(iCurImage->Pal.PalType);
  NewImage->Format = DestFormat;

  if (iGetBppFormat(NewImage->Format) == 0) {
    iCloseImage(NewImage);
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }

  //CurImage = iCurImage;
  //ilSetCurImage(NewImage);

  switch (DestFormat)
  {
    case IL_RGB:
      Converted = iConvertImagePal(NewImage, IL_PAL_RGB24);
      break;

    case IL_BGR:
      Converted = iConvertImagePal(NewImage, IL_PAL_BGR24);
      break;

    case IL_RGBA:
      Converted = iConvertImagePal(NewImage, IL_PAL_RGB32);
      break;

    case IL_BGRA:
      Converted = iConvertImagePal(NewImage, IL_PAL_BGR32);
      break;

    case IL_COLOUR_INDEX:
      // Just copy the original image over.
      NewImage->Data = (ILubyte*)ialloc(Image->SizeOfData);
      if (NewImage->Data == NULL)
        goto alloc_error;
      NewImage->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
      if (NewImage->Pal.Palette == NULL)
        goto alloc_error;
      memcpy(NewImage->Data, Image->Data, Image->SizeOfData);
      memcpy(NewImage->Pal.Palette, Image->Pal.Palette, Image->Pal.PalSize);
      NewImage->Pal.PalSize = Image->Pal.PalSize;
      NewImage->Pal.PalType = Image->Pal.PalType;
      return NewImage;

    default:
      iCloseImage(NewImage);
      iSetError(IL_INVALID_CONVERSION);
      return NULL;
  }

  // Resize to new bpp
  iResizeImage(NewImage, NewImage->Width, NewImage->Height, NewImage->Depth, iGetBppFormat(DestFormat), /*iGetBpcType(DestType)*/1);

  // ilConvertPal already sets the error message - no need to confuse the user.
  if (!Converted) {
    iCloseImage(NewImage);
    return NULL;
  }

  Size = iGetBppPal(NewImage->Pal.PalType);
  for (i = 0; i < Image->SizeOfData; i++) {
    for (c = 0; c < NewImage->Bpp; c++) {
      NewImage->Data[i * NewImage->Bpp + c] = NewImage->Pal.Palette[Image->Data[i] * Size + c];
    }
  }

  ifree(NewImage->Pal.Palette);

  NewImage->Pal.Palette = NULL;
  NewImage->Pal.PalSize = 0;
  NewImage->Pal.PalType = IL_PAL_NONE;

  return NewImage;

alloc_error:
  ifree(Temp);
  if (NewImage)
    iCloseImage(NewImage);
  return NULL;
}


// Converts an image from one format to another
ILAPI ILimage* ILAPIENTRY iConvertImage(ILimage *Image, ILenum DestFormat, ILenum DestType)
{
  ILimage *NewImage; //, *CurImage;
  ILuint  i;
  ILubyte *NewData;

  // CurImage = Image;
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }

  // We don't support 16-bit color indices (or higher).
  if (DestFormat == IL_COLOUR_INDEX && DestType >= IL_SHORT) {
    iSetError(IL_INVALID_CONVERSION);
    return NULL;
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    NewImage = iConvertPalette(Image, DestFormat);

    //added test 2003-09-01
    if (NewImage == NULL)
      return NULL;

    if (DestType == NewImage->Type)
      return NewImage;

    NewData = (ILubyte*)iConvertBuffer(NewImage->SizeOfData, NewImage->Format, DestFormat, NewImage->Type, DestType, NULL, NewImage->Data);
    if (NewData == NULL) {
      ifree(NewImage);  // iCloseImage not needed.
      return NULL;
    }
    ifree(NewImage->Data);
    NewImage->Data = NewData;

    iCopyImageAttr(NewImage, Image);
    NewImage->Format = DestFormat;
    NewImage->Type = DestType;
    NewImage->Bpc = iGetBpcType(DestType);
    NewImage->Bpp = iGetBppFormat(DestFormat);
    NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
    NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
    NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;
  }
  else if (DestFormat == IL_COLOUR_INDEX && Image->Format != IL_LUMINANCE) {
    if (iGetInt(IL_QUANTIZATION_MODE) == IL_NEU_QUANT)
      return iNeuQuant(Image, (ILuint)iGetInt(IL_MAX_QUANT_INDICES));
    else // Assume IL_WU_QUANT otherwise.
      return iQuantizeImage(Image, (ILuint)iGetInt(IL_MAX_QUANT_INDICES));
  }
  else {
    NewImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
    if (NewImage == NULL) {
      return NULL;
    }

    if (iGetBppFormat(DestFormat) == 0) {
      iSetError(IL_INVALID_PARAM);
      ifree(NewImage);
      return NULL;
    }

    iCopyImageAttr(NewImage, Image);
    NewImage->Format = DestFormat;
    NewImage->Type = DestType;
    NewImage->Bpc = iGetBpcType(DestType);
    NewImage->Bpp = iGetBppFormat(DestFormat);
    NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
    NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
    NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;

    if (DestFormat == IL_COLOUR_INDEX && Image->Format == IL_LUMINANCE) {
      NewImage->Pal.PalSize = 768;
      NewImage->Pal.PalType = IL_PAL_RGB24;
      NewImage->Pal.Palette = (ILubyte*)ialloc(768);
      for (i = 0; i < 256; i++) {
        NewImage->Pal.Palette[i * 3    ] = 
        NewImage->Pal.Palette[i * 3 + 1] = 
        NewImage->Pal.Palette[i * 3 + 2] = (ILubyte)i;
      }
      NewImage->Data = (ILubyte*)ialloc(Image->SizeOfData);
      if (NewImage->Data == NULL) {
        iCloseImage(NewImage);
        return NULL;
      }
      memcpy(NewImage->Data, Image->Data, Image->SizeOfData);
    }
    else {
      NewImage->Data = (ILubyte*)iConvertBuffer(Image->SizeOfData, Image->Format, DestFormat, Image->Type, DestType, NULL, Image->Data);
      if (NewImage->Data == NULL) {
        ifree(NewImage);  // iCloseImage not needed.
        return NULL;
      }
    }
  }

  return NewImage;
}


ILboolean ILAPIENTRY iConvertImages(ILimage *BaseImage, ILenum DestFormat, ILenum DestType) {
  if ( DestFormat == BaseImage->Format 
    && DestType   == BaseImage->Type )
    return IL_TRUE;  // No conversion needed.

  if (DestType == BaseImage->Type) {
    if (iFastConvert(BaseImage, DestFormat)) {
      BaseImage->Format = DestFormat;
      return IL_TRUE;
    }
  }

  if (ilIsEnabled(IL_USE_KEY_COLOUR)) {
    iAddAlphaKey(BaseImage);
  }

  while (BaseImage != NULL)
  {
    ILimage *Image = iConvertImage(BaseImage, DestFormat, DestType);
    if (Image == NULL)
      return IL_FALSE;

    //iCopyImageAttr(BaseImage, Image);  // Destroys subimages.

    // We don't copy the colour profile here, since it stays the same.
    //  Same with the DXTC data.
    BaseImage->Format       = DestFormat;
    BaseImage->Type         = DestType;
    BaseImage->Bpc          = iGetBpcType(DestType);
    BaseImage->Bpp          = iGetBppFormat(DestFormat);
    BaseImage->Bps          = BaseImage->Width * BaseImage->Bpc * BaseImage->Bpp;
    BaseImage->SizeOfPlane  = BaseImage->Bps * BaseImage->Height;
    BaseImage->SizeOfData   = BaseImage->Depth * BaseImage->SizeOfPlane;
    if (BaseImage->Pal.Palette && BaseImage->Pal.PalSize && BaseImage->Pal.PalType != IL_PAL_NONE)
      ifree(BaseImage->Pal.Palette);
    BaseImage->Pal.Palette = Image->Pal.Palette;
    BaseImage->Pal.PalSize = Image->Pal.PalSize;
    BaseImage->Pal.PalType = Image->Pal.PalType;
    Image->Pal.Palette = NULL;
    ifree(BaseImage->Data);
    BaseImage->Data = Image->Data;
    Image->Data = NULL;
    iCloseImage(Image);

    BaseImage = BaseImage->Next;
  }

  return IL_TRUE;
}



ILboolean ILAPIENTRY iSwapColours(ILimage *Image)
{
  ILuint    i = 0, Size;
  ILubyte   PalBpp;
  ILushort  *ShortPtr;
  ILuint    *IntPtr, Temp;
  ILdouble  *DoublePtr, DoubleTemp;
  void      *VoidPtr;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  PalBpp = (ILubyte)iGetBppPal(Image->Pal.PalType);
  Size = Image->Bpp * Image->Width * Image->Height;
  VoidPtr = Image->Data;

  if ((Image->Bpp != 1 && Image->Bpp != 3 && Image->Bpp != 4)) {
    iSetError(IL_INVALID_VALUE);
    return IL_FALSE;
  }

  // Just check before we change the format.
  if (Image->Format == IL_COLOUR_INDEX) {
    if (PalBpp == 0 || Image->Format != IL_COLOUR_INDEX) {
      iSetError(IL_ILLEGAL_OPERATION);
      return IL_FALSE;
    }
  }

  switch (Image->Format)
  {
    case IL_RGB:
      Image->Format = IL_BGR;
      break;
    case IL_RGBA:
      Image->Format = IL_BGRA;
      break;
    case IL_BGR:
      Image->Format = IL_RGB;
      break;
    case IL_BGRA:
      Image->Format = IL_RGBA;
      break;
    case IL_ALPHA:
    case IL_LUMINANCE:
    case IL_LUMINANCE_ALPHA:
      return IL_TRUE;  // No need to do anything to luminance or alpha images.
    case IL_COLOUR_INDEX:
      switch (Image->Pal.PalType)
      {
        case IL_PAL_RGB24:
          Image->Pal.PalType = IL_PAL_BGR24;
          break;
        case IL_PAL_RGB32:
          Image->Pal.PalType = IL_PAL_BGR32;
          break;
        case IL_PAL_RGBA32:
          Image->Pal.PalType = IL_PAL_BGRA32;
          break;
        case IL_PAL_BGR24:
          Image->Pal.PalType = IL_PAL_RGB24;
          break;
        case IL_PAL_BGR32:
          Image->Pal.PalType = IL_PAL_RGB32;
          break;
        case IL_PAL_BGRA32:
          Image->Pal.PalType = IL_PAL_RGBA32;
          break;
        default:
          iSetError(IL_ILLEGAL_OPERATION);
          return IL_FALSE;
      }
      break;
    default:
      iSetError(IL_ILLEGAL_OPERATION);
      return IL_FALSE;
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    for (; i < Image->Pal.PalSize; i += PalBpp) {
        Temp = Image->Pal.Palette[i];
        Image->Pal.Palette[i  ] = Image->Pal.Palette[i+2];
        Image->Pal.Palette[i+2] = (ILubyte)Temp;
    }
  } else {
    ShortPtr  = (ILushort*)VoidPtr;
    IntPtr    = (ILuint*)VoidPtr;
    DoublePtr = (ILdouble*)VoidPtr;
    switch (Image->Bpc)
    {
      case 1:
        for (; i < Size; i += Image->Bpp) {
          Temp = Image->Data[i];
          Image->Data[i  ] = Image->Data[i+2];
          Image->Data[i+2] = (ILubyte)Temp;
        }
        break;
      case 2:
        for (; i < Size; i += Image->Bpp) {
          Temp = ShortPtr[i];
          ShortPtr[i  ] = ShortPtr[i+2];
          ShortPtr[i+2] = (ILushort)Temp;
        }
        break;
      case 4:  // Works fine with ILint, ILuint and ILfloat.
        for (; i < Size; i += Image->Bpp) {
          Temp = IntPtr[i];
          IntPtr[i] = IntPtr[i+2];
          IntPtr[i+2] = Temp;
        }
        break;
      case 8:
        for (; i < Size; i += Image->Bpp) {
          DoubleTemp = DoublePtr[i];
          DoublePtr[i] = DoublePtr[i+2];
          DoublePtr[i+2] = DoubleTemp;
        }
        break;
    }
  }

  return IL_TRUE;
}

ILboolean iAddAlpha(ILimage *Image)
{
  void   *NewData;
  void   *OldData;
  ILubyte   NewBpp;
  ILuint    i = 0, j = 0, Size;

  if (ilIsEnabled(IL_USE_KEY_COLOUR)) {
    return iAddAlphaKey(Image);
  }

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Bpp != 3) {
    iSetError(IL_INVALID_VALUE);
    return IL_FALSE;
  }

  Size = Image->Bps * Image->Height / Image->Bpc;
  NewBpp = (ILubyte)(Image->Bpp + 1);
  
  NewData = ialloc(NewBpp * Image->Bpc * Image->Width * Image->Height);
  if (NewData == NULL) {
    return IL_FALSE;
  }
  OldData = Image->Data;

  switch (Image->Type)
  {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILubyte*)NewData)[j]   = ((ILubyte*)OldData)[i];
        ((ILubyte*)NewData)[j+1] = ((ILubyte*)OldData)[i+1];
        ((ILubyte*)NewData)[j+2] = ((ILubyte*)OldData)[i+2];
        ((ILubyte*)NewData)[j+3] = UCHAR_MAX;  // Max opaqueness
      }
      break;

    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILushort*)NewData)[j]   = ((ILushort*)OldData)[i];
        ((ILushort*)NewData)[j+1] = ((ILushort*)OldData)[i+1];
        ((ILushort*)NewData)[j+2] = ((ILushort*)OldData)[i+2];
        ((ILushort*)NewData)[j+3] = USHRT_MAX;
      }
      break;

    case IL_INT:
    case IL_UNSIGNED_INT:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILuint*)NewData)[j]   = ((ILuint*)OldData)[i];
        ((ILuint*)NewData)[j+1] = ((ILuint*)OldData)[i+1];
        ((ILuint*)NewData)[j+2] = ((ILuint*)OldData)[i+2];
        ((ILuint*)NewData)[j+3] = UINT_MAX;
      }
      break;

    case IL_FLOAT:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILfloat*)NewData)[j]   = ((ILfloat*)OldData)[i];
        ((ILfloat*)NewData)[j+1] = ((ILfloat*)OldData)[i+1];
        ((ILfloat*)NewData)[j+2] = ((ILfloat*)OldData)[i+2];
        ((ILfloat*)NewData)[j+3] = 1.0f;
      }
      break;

    case IL_DOUBLE:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILdouble*)NewData)[j]   = ((ILdouble*)OldData)[i];
        ((ILdouble*)NewData)[j+1] = ((ILdouble*)OldData)[i+1];
        ((ILdouble*)NewData)[j+2] = ((ILdouble*)OldData)[i+2];
        ((ILdouble*)NewData)[j+3] = 1.0;
      }
      break;

    default:
      ifree(NewData);
      iSetError(IL_INTERNAL_ERROR);
      return IL_FALSE;
  }


  Image->Bpp = NewBpp;
  Image->Bps = Image->Width * Image->Bpc * NewBpp;
  Image->SizeOfPlane = Image->Bps * Image->Height;
  Image->SizeOfData = Image->SizeOfPlane * Image->Depth;
  ifree(OldData); 
  OldData = NULL;
  Image->Data = NewData;

  switch (Image->Format)
  {
    case IL_RGB:
      Image->Format = IL_RGBA;
      break;
    case IL_BGR:
      Image->Format = IL_BGRA;
      break;
  }

  return IL_TRUE;
}


// Adds an alpha channel to an 8 or 24-bit image,
//  making the image transparent where Key is equal to the pixel.
ILboolean iAddAlphaKey(ILimage *Image)
{
  void   *NewData;
  ILubyte NewBpp;
  ILfloat   KeyColour[3];
  ILuint    i = 0, j = 0, c, Size;
  ILboolean Same;

  float KeyRed, KeyGreen, KeyBlue, KeyAlpha;
 
  iGetKeyColour(&KeyRed, &KeyGreen, &KeyBlue, &KeyAlpha);

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Format != IL_COLOUR_INDEX) {
    void *OldData = Image->Data;
    
    if (Image->Bpp != 3) {
      iSetError(IL_INVALID_VALUE);
      return IL_FALSE;
    }

    if (Image->Format == IL_BGR || Image->Format == IL_BGRA) {
      KeyColour[0] = KeyBlue;
      KeyColour[1] = KeyGreen;
      KeyColour[2] = KeyRed;
    }
    else {
      KeyColour[0] = KeyRed;
      KeyColour[1] = KeyGreen;
      KeyColour[2] = KeyBlue;
    }

    Size = Image->Bps * Image->Height / Image->Bpc;

    NewBpp = (ILubyte)(Image->Bpp + 1);
    
    NewData = (ILubyte*)ialloc(NewBpp * Image->Bpc * Image->Width * Image->Height);
    if (NewData == NULL) {
      return IL_FALSE;
    }

    switch (Image->Type)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
        for (; i < Size; i += Image->Bpp, j += NewBpp) {
          ((ILubyte*)NewData)[j]   = ((ILubyte*)OldData)[i];
          ((ILubyte*)NewData)[j+1] = ((ILubyte*)OldData)[i+1];
          ((ILubyte*)NewData)[j+2] = ((ILubyte*)OldData)[i+2];
          Same = IL_TRUE;
          for (c = 0; c < Image->Bpp; c++) {
            if (((ILubyte*)OldData)[i+c] != (ILubyte)(KeyColour[c] * UCHAR_MAX))
              Same = IL_FALSE;
          }

          if (Same)
            ((ILubyte*)NewData)[j+3] = 0;  // Transparent - matches key colour
          else
            ((ILubyte*)NewData)[j+3] = UCHAR_MAX;
        }
        break;

      case IL_SHORT:
      case IL_UNSIGNED_SHORT:
        for (; i < Size; i += Image->Bpp, j += NewBpp) {
          ((ILushort*)NewData)[j]   = ((ILushort*)OldData)[i];
          ((ILushort*)NewData)[j+1] = ((ILushort*)OldData)[i+1];
          ((ILushort*)NewData)[j+2] = ((ILushort*)OldData)[i+2];
          Same = IL_TRUE;
          for (c = 0; c < Image->Bpp; c++) {
            if (((ILushort*)OldData)[i+c] != (ILushort)(KeyColour[c] * USHRT_MAX))
              Same = IL_FALSE;
          }

          if (Same)
            ((ILushort*)NewData)[j+3] = 0;
          else
            ((ILushort*)NewData)[j+3] = USHRT_MAX;
        }
        break;

      case IL_INT:
      case IL_UNSIGNED_INT:
        for (; i < Size; i += Image->Bpp, j += NewBpp) {
          ((ILuint*)NewData)[j]   = ((ILuint*)OldData)[i];
          ((ILuint*)NewData)[j+1] = ((ILuint*)OldData)[i+1];
          ((ILuint*)NewData)[j+2] = ((ILuint*)OldData)[i+2];
          Same = IL_TRUE;
          for (c = 0; c < Image->Bpp; c++) {
            if (((ILuint*)OldData)[i+c] != (ILuint)(KeyColour[c] * UINT_MAX))
              Same = IL_FALSE;
          }

          if (Same)
            ((ILuint*)NewData)[j+3] = 0;
          else
            ((ILuint*)NewData)[j+3] = UINT_MAX;
        }
        break;

      case IL_FLOAT:
        for (; i < Size; i += Image->Bpp, j += NewBpp) {
          ((ILfloat*)NewData)[j]   = ((ILfloat*)OldData)[i];
          ((ILfloat*)NewData)[j+1] = ((ILfloat*)OldData)[i+1];
          ((ILfloat*)NewData)[j+2] = ((ILfloat*)OldData)[i+2];
          Same = IL_TRUE;
          for (c = 0; c < Image->Bpp; c++) {
            if (fabs( ((ILfloat*)OldData)[i+c] - KeyColour[c] ) < 1.0/255.0) 
              Same = IL_FALSE;
          }

          if (Same)
            ((ILfloat*)NewData)[j+3] = 0.0f;
          else
            ((ILfloat*)NewData)[j+3] = 1.0f;
        }
        break;

      case IL_DOUBLE:
        for (; i < Size; i += Image->Bpp, j += NewBpp) {
          ((ILdouble*)NewData)[j]   = ((ILdouble*)OldData)[i];
          ((ILdouble*)NewData)[j+1] = ((ILdouble*)OldData)[i+1];
          ((ILdouble*)NewData)[j+2] = ((ILdouble*)OldData)[i+2];
          Same = IL_TRUE;
          for (c = 0; c < Image->Bpp; c++) {
            if (fabs( ((ILdouble*)OldData)[i+c] - KeyColour[c] ) < 1.0/255.0) 
              Same = IL_FALSE;
          }

          if (Same)
            ((ILdouble*)NewData)[j+3] = 0.0;
          else
            ((ILdouble*)NewData)[j+3] = 1.0;
        }
        break;

      default:
        ifree(NewData);
        iSetError(IL_INTERNAL_ERROR);
        return IL_FALSE;
    }


    Image->Bpp = NewBpp;
    Image->Bps = Image->Width * Image->Bpc * NewBpp;
    Image->SizeOfPlane = Image->Bps * Image->Height;
    Image->SizeOfData = Image->SizeOfPlane * Image->Depth;
    ifree(OldData);
    OldData = NULL;
    Image->Data = NewData;

    switch (Image->Format)
    {
      case IL_RGB:
        Image->Format = IL_RGBA;
        break;
      case IL_BGR:
        Image->Format = IL_BGRA;
        break;
    }
  }
  else {  // IL_COLOUR_INDEX
    if (Image->Bpp != 1) {
      iSetError(IL_INTERNAL_ERROR);
      return IL_FALSE;
    }

    Size = (ILuint)iGetInteger(Image, IL_PALETTE_NUM_COLS);
    if (Size == 0) {
      iSetError(IL_INTERNAL_ERROR);
      return IL_FALSE;
    }

    if ((ILuint)(KeyAlpha * UCHAR_MAX) > Size) {
      iSetError(IL_INVALID_VALUE);
      return IL_FALSE;
    }

    switch (Image->Pal.PalType)
    {
      case IL_PAL_RGB24:
      case IL_PAL_RGB32:
      case IL_PAL_RGBA32:
        if (!iConvertImagePal(Image, IL_PAL_RGBA32))
          return IL_FALSE;
        break;
      case IL_PAL_BGR24:
      case IL_PAL_BGR32:
      case IL_PAL_BGRA32:
        if (!iConvertImagePal(Image, IL_PAL_BGRA32))
          return IL_FALSE;
        break;
      default:
        iSetError(IL_INTERNAL_ERROR);
        return IL_FALSE;
    }

    // Set the colour index to be transparent.
    Image->Pal.Palette[(ILuint)(KeyAlpha * UCHAR_MAX) * 4 + 3] = 0;

    // @TODO: Check if this is the required behaviour.

    if (Image->Pal.PalType == IL_PAL_RGBA32)
      iConvertImages(Image, IL_RGBA, IL_UNSIGNED_BYTE);
    else
      iConvertImages(Image, IL_BGRA, IL_UNSIGNED_BYTE);
  }

  return IL_TRUE;
}


ILboolean iRemoveAlpha(ILimage *Image)
{
  void *NewData, *OldData;
  ILubyte NewBpp;
  ILuint i = 0, j = 0, Size;

  if (Image == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  if (Image->Bpp != 4) {
    iSetError(IL_INVALID_VALUE);
    return IL_FALSE;
  }

  Size = Image->Bps * Image->Height;
  NewBpp = (ILubyte)(Image->Bpp - 1);
  
  NewData = (ILubyte*)ialloc(NewBpp * Image->Bpc * Image->Width * Image->Height);
  if (NewData == NULL) {
    return IL_FALSE;
  }
  OldData = Image->Data;

  switch (Image->Type)
  {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILubyte*)NewData)[j]   = ((ILubyte*)OldData)[i];
        ((ILubyte*)NewData)[j+1] = ((ILubyte*)OldData)[i+1];
        ((ILubyte*)NewData)[j+2] = ((ILubyte*)OldData)[i+2];
      }
      break;

    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILushort*)NewData)[j]   = ((ILushort*)OldData)[i];
        ((ILushort*)NewData)[j+1] = ((ILushort*)OldData)[i+1];
        ((ILushort*)NewData)[j+2] = ((ILushort*)OldData)[i+2];
      }
      break;

    case IL_INT:
    case IL_UNSIGNED_INT:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILuint*)NewData)[j]   = ((ILuint*)OldData)[i];
        ((ILuint*)NewData)[j+1] = ((ILuint*)OldData)[i+1];
        ((ILuint*)NewData)[j+2] = ((ILuint*)OldData)[i+2];
      }
      break;

    case IL_FLOAT:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILfloat*)NewData)[j]   = ((ILfloat*)OldData)[i];
        ((ILfloat*)NewData)[j+1] = ((ILfloat*)OldData)[i+1];
        ((ILfloat*)NewData)[j+2] = ((ILfloat*)OldData)[i+2];
      }
      break;

    case IL_DOUBLE:
      for (; i < Size; i += Image->Bpp, j += NewBpp) {
        ((ILdouble*)NewData)[j]   = ((ILdouble*)OldData)[i];
        ((ILdouble*)NewData)[j+1] = ((ILdouble*)OldData)[i+1];
        ((ILdouble*)NewData)[j+2] = ((ILdouble*)OldData)[i+2];
      }
      break;

    default:
      ifree(NewData);
      iSetError(IL_INTERNAL_ERROR);
      return IL_FALSE;
  }


  Image->Bpp = NewBpp;
  Image->Bps = Image->Width * Image->Bpc * NewBpp;
  Image->SizeOfPlane = Image->Bps * Image->Height;
  Image->SizeOfData = Image->SizeOfPlane * Image->Depth;
  ifree(OldData);
  OldData = NULL;
  Image->Data = NewData;

  switch (Image->Format)
  {
    case IL_RGBA:
      Image->Format = IL_RGB;
      break;
    case IL_BGRA:
      Image->Format = IL_BGR;
      break;
  }

  return IL_TRUE;
}

ILboolean iConvFloat16ToFloat32(ILuint* dest, ILushort* src, ILuint size)
{
  ILuint i;
  for (i = 0; i < size; ++i, ++dest, ++src) {
    //float: 1 sign bit, 8 exponent bits, 23 mantissa bits
    //half: 1 sign bit, 5 exponent bits, 10 mantissa bits
    *dest = ilHalfToFloat(*src);
  }

  return IL_TRUE;
}


// Same as iConvFloat16ToFloat32, but we have to set the blue channel to 1.0f.
//  The destination format is RGB, and the source is R16G16 (little endian).
ILboolean iConvG16R16ToFloat32(ILuint* dest, ILushort* src, ILuint size)
{
  ILuint i;
  for (i = 0; i < size; i += 3) {
    //float: 1 sign bit, 8 exponent bits, 23 mantissa bits
    //half: 1 sign bit, 5 exponent bits, 10 mantissa bits
    *dest++ = ilHalfToFloat(*src++);
    *dest++ = ilHalfToFloat(*src++);
    *((ILfloat*)dest++) = 1.0f;
  }

  return IL_TRUE;
}


// Same as iConvFloat16ToFloat32, but we have to set the green and blue channels
//  to 1.0f.  The destination format is RGB, and the source is R16.
ILboolean iConvR16ToFloat32(ILuint* dest, ILushort* src, ILuint size)
{
  ILuint i;
  for (i = 0; i < size; i += 3) {
    //float: 1 sign bit, 8 exponent bits, 23 mantissa bits
    //half: 1 sign bit, 5 exponent bits, 10 mantissa bits
    *dest++ = ilHalfToFloat(*src++);
    *((ILfloat*)dest++) = 1.0f;
    *((ILfloat*)dest++) = 1.0f;
  }

  return IL_TRUE;
}

static ILboolean iFixImage(ILimage *Image, ILimage *BaseImage) 
{
  Image->BaseImage = BaseImage;
  
  if (iIsEnabled(IL_ORIGIN_SET)) {
    if ((ILenum)iGetInteger(Image, IL_ORIGIN_MODE) != Image->Origin) {
      if (!iFlipImage(Image)) {
        return IL_FALSE;
      }
    }
  }

  if (iIsEnabled(IL_TYPE_SET)) {
    if ((ILenum)iGetInteger(Image, IL_TYPE_MODE) != Image->Type) {
      if (!iConvertImages(Image, Image->Format, (ILenum)iGetInteger(Image, IL_TYPE_MODE))) {
        return IL_FALSE;
      }
    }
  }
  if (iIsEnabled(IL_FORMAT_SET)) {
    if ((ILenum)iGetInteger(Image, IL_FORMAT_MODE) != Image->Format) {
      if (!iConvertImages(Image, (ILenum)iGetInteger(Image, IL_FORMAT_MODE), Image->Type)) {
        return IL_FALSE;
      }
    }
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    if (iIsEnabled(IL_CONV_PAL)) {
      if (!iConvertImages(Image, IL_BGR, IL_UNSIGNED_BYTE)) {
        return IL_FALSE;
      }
    }
  }
  
  return IL_TRUE;
}

/*
This function was replaced 20050304, because the previous version
didn't fix the mipmaps of the subimages etc. This version is not
completely correct either, because the subimages of the subimages
etc. are not fixed, but at the moment no images of this type can
be loaded anyway. Thanks to Chris Lux for pointing this out.
*/

// BP: rewritten to recurse into each and every mipmap, face, and layer

ILboolean iFixImages(ILimage *Image, ILimage *BaseImage) {
  while(Image) {
    if (!iFixImage(Image, BaseImage))
      return IL_FALSE;

    if (!iFixImages(Image->Mipmaps, BaseImage))
      return IL_FALSE;

    if (!iFixImages(Image->Layers, BaseImage))
      return IL_FALSE;

    if (!iFixImages(Image->Faces, BaseImage))
      return IL_FALSE;

    Image = Image->Next;
  }

  return IL_TRUE;
}


