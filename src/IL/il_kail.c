//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/01/2009
//
// Filename: src-IL/src/il_devil.c
//
// Description: Functions for working with the ILimage's and the current image
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include <string.h>
#include <limits.h>
#include "il_manip.h"

// Internal version of ilTexImage.
ILAPI ILboolean ILAPIENTRY iTexImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, 
  ILenum Format, ILenum Type, void *Data);

ILAPI ILboolean ILAPIENTRY iInitImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, 
  ILenum Format, ILenum Type, void *Data)
{
  memset(Image, 0, sizeof(ILimage));

  Image->Origin      = IL_ORIGIN_LOWER_LEFT;
  Image->Pal.PalType = IL_PAL_NONE;
  Image->Duration    = 0;
  Image->DxtcFormat  = IL_DXT_NO_COMP;
  Image->DxtcData    = NULL;
  Image->BaseImage   = Image; // for locking

  iResetWrite(Image);
  iResetRead(Image);

  return iTexImage(Image, Width, Height, Depth, Bpp, Format, Type, Data);
}


// Creates a new ILimage based on the specifications given
ILAPI ILimage* ILAPIENTRY iNewImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc)
{
  ILimage *Image;

  if (Bpp == 0 || Bpp > 4) {
    return NULL;
  }

  Image = (ILimage*)ialloc(sizeof(ILimage));
  if (Image == NULL) {
    return NULL;
  }

  if (!iInitImage(Image, Width, Height, Depth, Bpp, iGetFormatBpp(Bpp), iGetTypeBpc(Bpc), NULL)) {
    if (Image->Data != NULL) {
      ifree(Image->Data);
    }
    ifree(Image);
    return NULL;
  }
  
  return Image;
}


// Same as above but allows specification of Format and Type
ILAPI ILimage* ILAPIENTRY iNewImageFull(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data)
{
  ILimage *Image;

  if (Bpp == 0 || Bpp > 4) {
    return NULL;
  }

  Image = (ILimage*)ialloc(sizeof(ILimage));
  if (Image == NULL) {
    return NULL;
  }

  if (!iInitImage(Image, Width, Height, Depth, Bpp, Format, Type, Data)) {
    if (Image->Data != NULL) {
      ifree(Image->Data);
    }
    ifree(Image);
    return NULL;
  }
  
  return Image;
}


// Internal version of ilTexImage.
// Differs from ilTexImage in the first argument: ILimage* Image
// Defined as ILAPIENTRY because it's in internal exports.
ILAPI ILboolean ILAPIENTRY iTexImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, 
  ILenum Format, ILenum Type, void *Data)
{
  ILubyte BpcType = iGetBpcType(Type);

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (BpcType == 0) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }

  ////

  // Not sure if we should be getting rid of the palette...
  if (Image->Pal.Palette && Image->Pal.PalSize && Image->Pal.PalType != IL_PAL_NONE) {
    ifree(Image->Pal.Palette);
  }

  iCloseImage(Image->Mipmaps); Image->Mipmaps  = NULL;
  iCloseImage(Image->Next);    Image->Next     = NULL;
  iCloseImage(Image->Faces);   Image->Faces    = NULL;
  iCloseImage(Image->Layers);  Image->Layers   = NULL;

  if (Image->Profile)   { ifree(Image->Profile);  Image->Profile  = NULL; }
  if (Image->DxtcData)  { ifree(Image->DxtcData); Image->DxtcData = NULL; }
  if (Image->Data)      { ifree(Image->Data);     Image->Data     = NULL; }

  ////

  //@TODO: Also check against format?
  /*if (Width == 0 || Height == 0 || Depth == 0 || Bpp == 0) {
    iSetError(IL_INVALID_PARAM);
  return IL_FALSE;
  }*/

  ////
  if (Width  == 0) Width = 1;
  if (Height == 0) Height = 1;
  if (Depth  == 0) Depth = 1;
  Image->Width     = Width;
  Image->Height    = Height;
  Image->Depth     = Depth;
  Image->Bpp       = Bpp;
  Image->Bpc       = BpcType;
  Image->Bps       = Width * Bpp * Image->Bpc;
  Image->SizeOfPlane = Image->Bps * Height;
  Image->SizeOfData  = Image->SizeOfPlane * Depth;
  Image->Format      = Format;
  Image->Type        = Type;
  Image->Data = (ILubyte*)ialloc(Image->SizeOfData);

  if (Image->Data != NULL) {
    if (Data != NULL) {
      memcpy(Image->Data, Data, Image->SizeOfData);
    }

    return IL_TRUE;
  } else {
    return IL_FALSE;
  }
}

// Internal version of ilSetData.
ILboolean iSetData(ILimage *Image, void *Data) {
  if (Image == NULL || Data == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }
  if (!Image->Data) {
    Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
    if (Image->Data == NULL)
      return IL_FALSE;
  }
  memcpy(Image->Data, Data, Image->SizeOfData);
  return IL_TRUE;
}


ILubyte* iGetData(ILimage *Image) {
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }
  
  return Image->Data;
}

ILubyte *iGetPalette(ILimage *Image) {
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }
  return Image->Pal.Palette;
}

ILboolean ILAPIENTRY iClearImage(ILimage *Image)
{
  ILuint    i, c, NumBytes;
  ILubyte   Colours[32];  // Maximum is sizeof(double) * 4 = 32
  ILubyte   *BytePtr;
  ILushort  *ShortPtr;
  ILuint    *IntPtr;
  ILfloat   *FloatPtr;
  ILdouble  *DblPtr;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }
  
  NumBytes = Image->Bpp * Image->Bpc;
  iGetClear(Colours, Image->Format, Image->Type);
  
  if (Image->Format != IL_COLOUR_INDEX) {
    switch (Image->Type)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
        BytePtr = (ILubyte*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            Image->Data[i] = BytePtr[c];
          }
        }
          break;
        
      case IL_SHORT:
      case IL_UNSIGNED_SHORT:
        ShortPtr = (ILushort*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILushort*)(Image->Data + i)) = ShortPtr[c / Image->Bpc];
          }
        }
          break;
        
      case IL_INT:
      case IL_UNSIGNED_INT:
        IntPtr = (ILuint*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILuint*)(Image->Data + i)) = IntPtr[c / Image->Bpc];
          }
        }
          break;
        
      case IL_FLOAT:
        FloatPtr = (ILfloat*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILfloat*)(Image->Data + i)) = FloatPtr[c / Image->Bpc];
          }
        }
          break;
        
      case IL_DOUBLE:
        DblPtr = (ILdouble*)Colours;
        for (c = 0; c < NumBytes; c += Image->Bpc) {
          for (i = c; i < Image->SizeOfData; i += NumBytes) {
            *((ILdouble*)(Image->Data + i)) = DblPtr[c / Image->Bpc];
          }
        }
          break;
    }
  }
  else {
    imemclear(Image->Data, Image->SizeOfData);
    
    if (Image->Pal.Palette)
      ifree(Image->Pal.Palette);
    Image->Pal.Palette = (ILubyte*)ialloc(4);
    if (Image->Pal.Palette == NULL) {
      return IL_FALSE;
    }
    
    Image->Pal.PalType = IL_PAL_RGBA32;
    Image->Pal.PalSize = 4;
    
    Image->Pal.Palette[0] = Colours[0] * UCHAR_MAX;
    Image->Pal.Palette[1] = Colours[1] * UCHAR_MAX;
    Image->Pal.Palette[2] = Colours[2] * UCHAR_MAX;
    Image->Pal.Palette[3] = Colours[3] * UCHAR_MAX;
  }
  
  return IL_TRUE;
}

ILboolean iBlit(ILimage *Dest, ILimage *Src, ILint DestX,  ILint DestY,   ILint DestZ, 
                                           ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
                                           ILuint Width, ILuint Height, ILuint Depth)
{
  ILuint    x, y, z, ConvBps, ConvSizePlane;
  ILubyte   *Converted;
  ILuint    c;
  // ILuint   StartX, StartY, StartZ;
  ILboolean DestFlipped = IL_FALSE;
  ILboolean DoAlphaBlend = IL_FALSE;
  ILubyte   *SrcTemp;
  ILfloat   ResultAlpha;

  // Check if the destination and source really exist
  if (Dest == NULL || Src == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  // set the destination image to upper left origin
  if (Dest->Origin == IL_ORIGIN_LOWER_LEFT) {  // Dest
    DestFlipped = IL_TRUE;
    iFlipImage(Dest);
  }
  //determine alpha support
  DoAlphaBlend = ilIsEnabled(IL_BLIT_BLEND);

  //@TODO test if coordinates are inside the images (hard limit for source)
  
  // set the source image to upper left origin
  if (Src->Origin == IL_ORIGIN_LOWER_LEFT)
  {
    SrcTemp = iGetFlipped(Src);
    if (SrcTemp == NULL)
    {
      if (DestFlipped)
        iFlipImage(Dest);
      return IL_FALSE;
    }
  }
  else
  {
    SrcTemp = Src->Data;
  }
  
  // convert source image to match the destination image type and format
  Converted = (ILubyte*)iConvertBuffer(Src->SizeOfData, Src->Format, Dest->Format, Src->Type, Dest->Type, NULL, SrcTemp);
  if (Converted == NULL)
    return IL_FALSE;
  
  ConvBps       = Dest->Bpp * Src->Width;
  ConvSizePlane = ConvBps   * Src->Height;
  
  // if dest is negative, adjust src and dest accordingly to blit in the
  // overlapped region only
  if (DestX < 0) {
    if (Width <= (ILuint)-DestX) return IL_TRUE; // nothing to do
    Width -= DestX;
    DestX = 0;
  }

  if (DestY < 0) {
    if (Height <= (ILuint)-DestY) return IL_TRUE; // nothing to do
    Height -= DestY;
    DestY = 0;
  }

  if (DestZ < 0) {
    if (Depth <= (ILuint)-DestZ) return IL_TRUE; // nothing to do
    Depth -= DestZ;
    DestZ = 0;
  }
  
  // Limit the copy of data inside of the destination image
  if (Width  + DestX > Dest->Width)  Width  = Dest->Width  - DestX;
  if (Height + DestY > Dest->Height) Height = Dest->Height - DestY;
  if (Depth  + DestZ > Dest->Depth)  Depth  = Dest->Depth  - DestZ;
  
  //@TODO: non funziona con rgba
  if (Src->Format == IL_RGBA || Src->Format == IL_BGRA || Src->Format == IL_LUMINANCE_ALPHA) {
    const ILuint bpp_without_alpha = Dest->Bpp - 1;
    for (z = 0; z < Depth; z++) {
      for (y = 0; y < Height; y++) {
        for (x = 0; x < Width; x++) {
          const ILuint  SrcIndex  = (z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*Dest->Bpp;
          const ILuint  DestIndex = (z+DestZ)*Dest->SizeOfPlane + (y+DestY)*Dest->Bps + (x+DestX)*Dest->Bpp;
          const ILuint  AlphaIdx = SrcIndex + bpp_without_alpha;
          ILfloat FrontAlpha = 0; // foreground opacity
          ILfloat BackAlpha = 0;  // background opacity
          
          switch (Dest->Type)
          {
            case IL_BYTE:
            case IL_UNSIGNED_BYTE:
              FrontAlpha = Converted[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
              BackAlpha = Dest->Data[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
              break;
            case IL_SHORT:
            case IL_UNSIGNED_SHORT:
              FrontAlpha = ((ILshort*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
              BackAlpha = ((ILshort*)Dest->Data)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
              break;
            case IL_INT:
            case IL_UNSIGNED_INT:
              FrontAlpha = ((ILint*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
              BackAlpha = ((ILint*)Dest->Data)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
              break;
            case IL_FLOAT:
              FrontAlpha = ((ILfloat*)Converted)[AlphaIdx];
              BackAlpha = ((ILfloat*)Dest->Data)[AlphaIdx];
              break;
            case IL_DOUBLE:
              FrontAlpha = (ILfloat)(((ILdouble*)Converted)[AlphaIdx]);
              BackAlpha = (ILfloat)(((ILdouble*)Dest->Data)[AlphaIdx]);
              break;
          }
          
          // In case of Alpha channel, the data is blended.
          // Computes composite Alpha
          if (DoAlphaBlend)
          {
            ResultAlpha = FrontAlpha + (1.0f - FrontAlpha) * BackAlpha;
            for (c = 0; c < bpp_without_alpha; c++)
            {
              Dest->Data[DestIndex + c] = (ILubyte)( 0.5f + 
                (Converted[SrcIndex + c] * FrontAlpha + 
                (1.0f - FrontAlpha) * Dest->Data[DestIndex + c] * BackAlpha) 
                / ResultAlpha);
            }
            Dest->Data[AlphaIdx] = (ILubyte)(0.5f + ResultAlpha * (float)IL_MAX_UNSIGNED_BYTE);
          }
          else {
            for (c = 0; c < Dest->Bpp; c++)
            {
              Dest->Data[DestIndex + c] = (ILubyte)(Converted[SrcIndex + c]);
            }
          }
        }
      }
    }
  } else {
    for( z = 0; z < Depth; z++ ) {
      for( y = 0; y < Height; y++ ) {
        for( x = 0; x < Width; x++ ) {
          for( c = 0; c < Dest->Bpp; c++ ) {
            Dest->Data[(z+DestZ)*Dest->SizeOfPlane + (y+DestY)*Dest->Bps + (x+DestX)*Dest->Bpp + c] =
             Converted[(z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*Dest->Bpp + c];
          }
        }
      }
    }
  }
  
  if (SrcTemp != Src->Data)
    ifree(SrcTemp);
  
  if (DestFlipped)
    iFlipImage(Dest);
  
  ifree(Converted);
  return IL_TRUE;
}

ILboolean iOverlayImage(ILimage *Dest, ILimage *Src, ILint XCoord, ILint YCoord, ILint ZCoord) {
  ILuint  Width, Height, Depth;

  // Check if the destination and source really exist
  if (Dest == NULL || Src == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Width = Dest->Width;  
  Height = Dest->Height;  
  Depth = Dest->Depth;

  return iBlit(Dest, Src, XCoord, YCoord, ZCoord, 0, 0, 0, Width, Height, Depth);
}

ILboolean iCopySubImage(ILimage *Dest, ILimage *Src)
{
  ILimage *DestTemp, *SrcTemp;
  
  DestTemp = Dest;
  SrcTemp = Src;
  
  do {
    iCopyImageAttr(DestTemp, SrcTemp);
    DestTemp->Data = (ILubyte*)ialloc(SrcTemp->SizeOfData);
    if (DestTemp->Data == NULL) {
      return IL_FALSE;
    }
    memcpy(DestTemp->Data, SrcTemp->Data, SrcTemp->SizeOfData);
    
    if (SrcTemp->Next) {
      DestTemp->Next = (ILimage*)icalloc(1, sizeof(ILimage));
      if (!DestTemp->Next) {
        return IL_FALSE;
      }
    }
    else {
      DestTemp->Next = NULL;
    }
    
    DestTemp = DestTemp->Next;
    SrcTemp = SrcTemp->Next;
  } while (SrcTemp);
  
  return IL_TRUE;
}


ILboolean iCopySubImages(ILimage *Dest, ILimage *Src)
{
  if (Src->Faces) {
    Dest->Faces = (ILimage*)icalloc(1, sizeof(ILimage));
    if (!Dest->Faces) {
      return IL_FALSE;
    }
    if (!iCopySubImage(Dest->Faces, Src->Faces))
      return IL_FALSE;
  }
  
  if (Src->Layers) {
    Dest->Layers = (ILimage*)icalloc(1, sizeof(ILimage));
    if (!Dest->Layers) {
      return IL_FALSE;
    }
    if (!iCopySubImage(Dest->Layers, Src->Layers))
      return IL_FALSE;
  }

  if (Src->Mipmaps) {
    Dest->Mipmaps = (ILimage*)icalloc(1, sizeof(ILimage));
    if (!Dest->Mipmaps) {
      return IL_FALSE;
    }
    if (!iCopySubImage(Dest->Mipmaps, Src->Mipmaps))
      return IL_FALSE;
  }

  if (Src->Next) {
    Dest->Next = (ILimage*)icalloc(1, sizeof(ILimage));
    if (!Dest->Next) {
      return IL_FALSE;
    }
    if (!iCopySubImage(Dest->Next, Src->Next))
      return IL_FALSE;
  }

  return IL_TRUE;
}


// Copies everything but the Data from Src to Dest.
ILAPI ILboolean ILAPIENTRY iCopyImageAttr(ILimage *Dest, ILimage *Src)
{
  if (Dest == NULL || Src == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }
  
  if (Dest->Pal.Palette && Dest->Pal.PalSize && Dest->Pal.PalType != IL_PAL_NONE) {
    ifree(Dest->Pal.Palette);
    Dest->Pal.Palette = NULL;
  }
  if (Dest->Faces) {
    iCloseImage(Dest->Faces);
    Dest->Faces = NULL;
  }
  if (Dest->Layers) {
    iCloseImage(Dest->Layers);
    Dest->Layers = NULL;
  }
  if (Dest->Mipmaps) {
    iCloseImage(Dest->Mipmaps);
    Dest->Mipmaps = NULL;
  }
  if (Dest->Next) {
    iCloseImage(Dest->Next);
    Dest->Next = NULL;
  }
  if (Dest->Profile) {
    ifree(Dest->Profile);
    Dest->Profile = NULL;
    Dest->ProfileSize = 0;
  }
  if (Dest->DxtcData) {
    ifree(Dest->DxtcData);
    Dest->DxtcData = NULL;
    Dest->DxtcFormat = IL_DXT_NO_COMP;
    Dest->DxtcSize = 0;
  }
  
  if (Src->Profile) {
    Dest->Profile = (ILubyte*)ialloc(Src->ProfileSize);
    if (Dest->Profile == NULL) {
      return IL_FALSE;
    }
    memcpy(Dest->Profile, Src->Profile, Src->ProfileSize);
    Dest->ProfileSize = Src->ProfileSize;
  }
  if (Src->Pal.Palette) {
    Dest->Pal.Palette = (ILubyte*)ialloc(Src->Pal.PalSize);
    if (Dest->Pal.Palette == NULL) {
      return IL_FALSE;
    }
    memcpy(Dest->Pal.Palette, Src->Pal.Palette, Src->Pal.PalSize);
  }
  else {
    Dest->Pal.Palette = NULL;
  }

  // TODO: exif
  
  Dest->Pal.PalSize = Src->Pal.PalSize;
  Dest->Pal.PalType = Src->Pal.PalType;
  Dest->Width = Src->Width;
  Dest->Height = Src->Height;
  Dest->Depth = Src->Depth;
  Dest->Bpp = Src->Bpp;
  Dest->Bpc = Src->Bpc;
  Dest->Bps = Src->Bps;
  Dest->SizeOfPlane = Src->SizeOfPlane;
  Dest->SizeOfData = Src->SizeOfData;
  Dest->Format = Src->Format;
  Dest->Type = Src->Type;
  Dest->Origin = Src->Origin;
  Dest->Duration = Src->Duration;
  Dest->CubeFlags = Src->CubeFlags;
  Dest->OffX = Src->OffX;
  Dest->OffY = Src->OffY;
  
  return IL_TRUE/*iCopySubImages(Dest, Src)*/;
}


ILboolean ILAPIENTRY iCopyImage(ILimage *DestImage, ILimage *SrcImage)
{
  if (SrcImage == NULL || SrcImage == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }
  
  iTexImage(DestImage, SrcImage->Width, SrcImage->Height, SrcImage->Depth, SrcImage->Bpp, SrcImage->Format, SrcImage->Type, SrcImage->Data);
  iCopyImageAttr(DestImage, SrcImage);
  
  return IL_TRUE;
}


// Creates a copy of Src and returns it.
// TODO: rename to iCloneImage
ILAPI ILimage* ILAPIENTRY iCloneImage(ILimage *Src)
{
  ILimage *Dest;
  
  if (Src == NULL) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }
  
  Dest = iNewImage(Src->Width, Src->Height, Src->Depth, Src->Bpp, Src->Bpc);
  if (Dest == NULL) {
    return NULL;
  }
  
  if (iCopyImageAttr(Dest, Src) == IL_FALSE)
    return NULL;
  
  memcpy(Dest->Data, Src->Data, Src->SizeOfData);
  
  return Dest;
}


ILuint iDuplicateImage(ILimage *SrcImage) {
  ILuint  DestName;
  ILimage *DestImage;
  
  if (SrcImage == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return 0;
  }
  
  ilGenImages(1, &DestName);
  if (DestName == 0)
    return 0;
  
  DestImage = iLockImage(DestName);
  iTexImage(DestImage, SrcImage->Width, SrcImage->Height, SrcImage->Depth, SrcImage->Bpp, SrcImage->Format, SrcImage->Type, SrcImage->Data);
  iCopyImageAttr(DestImage, SrcImage);
  iUnlockImage(DestImage);
  return DestName;
}


// Like ilTexImage but doesn't destroy the palette.
ILAPI ILboolean ILAPIENTRY iResizeImage(ILimage *Image, ILuint Width, ILuint Height, 
  ILuint Depth, ILubyte Bpp, ILubyte Bpc)
{
  if (Image == NULL) {
    iSetError(IL_INVALID_PARAM);
    return IL_FALSE;
  }
  
  if (Image->Data != NULL)
    ifree(Image->Data);
  
  Image->Depth        = Depth;
  Image->Width        = Width;
  Image->Height       = Height;
  Image->Bpp          = Bpp;
  Image->Bpc          = Bpc;
  Image->Bps          = Bpp * Bpc * Width;
  Image->SizeOfPlane  = Image->Bps * Height;
  Image->SizeOfData   = Image->SizeOfPlane * Depth;
  
  Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
  if (Image->Data == NULL) {
    return IL_FALSE;
  }
  
  return IL_TRUE;
}
