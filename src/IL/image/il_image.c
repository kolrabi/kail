#include "il_internal.h"

// Set sane default values
ILAPI ILboolean ILAPIENTRY iInitImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data) {
  imemclear(Image, sizeof(ILimage));

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

// Same as above but allows specification of Format and Type
ILAPI ILimage* ILAPIENTRY iNewImageFull(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data) {
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

// Creates a new ILimage based on the specifications given
ILAPI ILimage* ILAPIENTRY iNewImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc) {
  return iNewImageFull(Width, Height, Depth, Bpp, iGetFormatBpp(Bpp), iGetTypeBpc(Bpc), NULL);
}

// Internal version of ilTexImage.
// Defined as ILAPIENTRY because it's in internal exports.
ILAPI ILboolean ILAPIENTRY iTexImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data) {
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
  Image->Width        = Width;
  Image->Height       = Height;
  Image->Depth        = Depth;
  Image->Bpp          = Bpp;
  Image->Bpc          = BpcType;
  Image->Bps          = Width * Bpp * Image->Bpc;
  Image->SizeOfPlane  = Image->Bps * Height;
  Image->SizeOfData   = Image->SizeOfPlane * Depth;
  Image->Format       = Format;
  Image->Type         = Type;
  Image->Data         = (ILubyte*)ialloc(Image->SizeOfData);

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

// Internal version of ilGetData
ILubyte* iGetData(ILimage *Image) {
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }
  
  return Image->Data;
}

// Internal version of ilGetPalette
ILubyte *iGetPalette(ILimage *Image) {
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }
  return Image->Pal.Palette;
}

// Creates a copy of Src and returns it.
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
