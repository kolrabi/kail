#include "il_internal.h"

// FIXME: actually use these functions!
/*
static ILboolean iCopySubImage(ILimage *Dest, ILimage *Src)
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


static ILboolean iCopySubImages(ILimage *Dest, ILimage *Src)
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
*/

// Copies everything but the Data and MetaTags from Src to Dest.
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
  if (DestImage == NULL || SrcImage == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }
  
  iTexImage(DestImage, SrcImage->Width, SrcImage->Height, SrcImage->Depth, SrcImage->Bpp, SrcImage->Format, SrcImage->Type, SrcImage->Data);
  iCopyImageAttr(DestImage, SrcImage);
  
  return IL_TRUE;
}
