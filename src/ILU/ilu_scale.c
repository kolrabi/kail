//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 12/27/2008
//
// Filename: src-ILU/src/ilu_scale.c
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"


ILboolean iScale(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILenum Filter) {
  ILimage   *Temp;
  ILboolean UsePal;
  ILenum    PalType;
  ILenum    Origin;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Width == Width && Image->Height == Height && Image->Depth == Depth)
    return IL_TRUE;

  // A parameter of 0 is not valid.  Let's just assume that the user wanted a value of 1 instead.
  if (Width == 0)  Width = 1;
  if (Height == 0) Height = 1;
  if (Depth == 0)  Depth = 1;

  if ((Image->Width<Width) || (Image->Height<Height)) // only do special scale if there is some zoom?
  {
    switch (Filter)
    {
      case ILU_SCALE_BOX:
      case ILU_SCALE_TRIANGLE:
      case ILU_SCALE_BELL:
      case ILU_SCALE_BSPLINE:
      case ILU_SCALE_LANCZOS3:
      case ILU_SCALE_MITCHELL:

        // Not supported yet.
        if ( Image->Type   != IL_UNSIGNED_BYTE
          || Image->Format == IL_COLOUR_INDEX
          || Image->Depth > 1) {
            iSetError(ILU_ILLEGAL_OPERATION);
            return IL_FALSE;
        }

        if (Image->Width > Width) // shrink width first
        {
          Origin = Image->Origin;
          Temp = iluScale_(Image, Width, Image->Height, Image->Depth, Filter);
          if (Temp != NULL) {
            if (!iTexImage(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
              iCloseImage(Temp);
              return IL_FALSE;
            }
            Image->Origin = Origin;
            iCloseImage(Temp);
          }
        }
        else if (Image->Height > Height) // shrink height first
        {
          Origin = Image->Origin;
          Temp = iluScale_(Image, Image->Width, Height, Image->Depth, Filter);
          if (Temp != NULL) {
            if (!iTexImage(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
              iCloseImage(Temp);
              return IL_FALSE;
            }
            Image->Origin = Origin;
            iCloseImage(Temp);
          }
        }

        return (ILboolean)iScaleAdvanced(Image, Width, Height, Filter);
    }
  }

  Origin = Image->Origin;
  UsePal = (Image->Format == IL_COLOUR_INDEX);
  PalType = Image->Pal.PalType;
  Temp = iluScale_(Image, Width, Height, Depth, Filter);
  if (Temp != NULL) {
    if (!iTexImage(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
      iCloseImage(Temp);
      return IL_FALSE;
    }
    Image->Origin = Origin;
    iCloseImage(Temp);
    if (UsePal) {
      if (!iConvertImages(Image, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE))
        return IL_FALSE;
      if (!iConvertImagePal(Image, PalType))
        return IL_FALSE;
    }
    return IL_TRUE;
  }

  return IL_FALSE;
}


ILAPI ILimage* ILAPIENTRY iluScale_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILenum Filter)
{
  ILimage *Scaled, *ToScale;
  ILenum  Format; // , PalType;

  Format = Image->Format;
  if (Format == IL_COLOUR_INDEX) {
    // PalType = Image->Pal.PalType;
    ToScale = iConvertImage(Image, iGetPalBaseType(Image->Pal.PalType), Image->Type);
  }
  else {
    ToScale = Image;
  }

  // So we don't replicate this 3 times (one in each iluScalexD_() function.
  Scaled = (ILimage*)icalloc(1, sizeof(ILimage));
  if (iCopyImageAttr(Scaled, ToScale) == IL_FALSE) {
    iCloseImage(Scaled);
    if (ToScale != Image)
      iCloseImage(ToScale);
    return NULL;
  }
  if (iResizeImage(Scaled, Width, Height, Depth, ToScale->Bpp, ToScale->Bpc) == IL_FALSE) {
    iCloseImage(Scaled);
    if (ToScale != Image)
      iCloseImage(ToScale);
    return NULL;
  }
  
  if (Height <= 1 && Image->Height <= 1) {
    iluScale1D_(ToScale, Scaled, Width, Filter);
  }
  if (Depth <= 1 && Image->Depth <= 1) {
    iluScale2D_(ToScale, Scaled, Width, Height, Filter);
  }
  else {
    iluScale3D_(ToScale, Scaled, Width, Height, Depth, Filter);
  }

  if (ToScale != Image)
    iCloseImage(ToScale);

  return Scaled;
}


ILimage *iluScale1D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILenum Filter)
{
  ILuint    NewX1, NewX2, x;
  ILdouble  ScaleX;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return NULL;
  }

  ScaleX = (ILdouble)Width / Image->Width;

  if (Filter == ILU_NEAREST) {
    for (x = 0; x < Width; x++) {
      NewX1 = x * Scaled->Bpp * Scaled->Bpc;
      NewX2 = (ILuint)(x / ScaleX) * Image->Bpp * Image->Bpc;
      memcpy(Scaled->Data + NewX1, Image->Data + NewX2, Scaled->Bpp * Scaled->Bpc);
    }
  } else {  // IL_LINEAR or IL_BILINEAR
    ILuint NewX3, c;
    switch (Image->Bpc)
    {
      case 1:
        NewX3 = 0;
        for (x = 0; x < Width; x++) {
          ILdouble t1 = x / (ILdouble)Width;
          ILdouble t2 = t1 * Width - (ILuint)(t1 * Width);
          ILdouble f = (1.0 - cos(t2 * IL_PI)) * .5;
          NewX1 = ((ILuint)(t1 * Width / ScaleX)) * Image->Bpp;
          NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * Image->Bpp;

          for (c = 0; c < Scaled->Bpp; c++) {
            ILubyte x1 = Image->Data[NewX1 + c];
            ILubyte x2 = Image->Data[NewX2 + c];

            Scaled->Data[NewX3 + c] = (ILubyte)(x1 * (1.0 - f) + x2 * f);
          }

          NewX3 += Scaled->Bpp;
        }
        break;
      case 2:
        NewX3 = 0;
        for (x = 0; x < Width; x++) {
          ILdouble t1 = x / (ILdouble)Width;
          ILdouble t2 = t1 * Width - (ILuint)(t1 * Width);
          ILdouble f = (1.0 - cos(t2 * IL_PI)) * .5;
          NewX1 = ((ILuint)(t1 * Width / ScaleX)) * Image->Bpp;
          NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * Image->Bpp;

          for (c = 0; c < Scaled->Bpp; c++) {
            ILushort x1 = iGetImageDataUShort(Image)[NewX1 + c];
            ILushort x2 = iGetImageDataUShort(Image)[NewX2 + c];

            iGetImageDataUShort(Scaled)[NewX3 + c] = (ILushort)(x1 * (1.0 - f) + x2 * f);
          }

          NewX3 += Scaled->Bpp;
        }
        break;
      case 4:
        NewX3 = 0;
        for (x = 0; x < Width; x++) {
          ILdouble t1 = x / (ILdouble)Width;
          ILdouble t2 = t1 * Width - (ILuint)(t1 * Width);
          ILdouble f = (1.0 - cos(t2 * IL_PI)) * .5;
          NewX1 = ((ILuint)(t1 * Width / ScaleX)) * Image->Bpp;
          NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * Image->Bpp;

          for (c = 0; c < Scaled->Bpp; c++) {
            ILuint x1 = iGetImageDataUInt(Image)[NewX1 + c];
            ILuint x2 = iGetImageDataUInt(Image)[NewX2 + c];

            iGetImageDataUInt(Scaled)[NewX3 + c] = (ILuint)(x1 * (1.0 - f) + x2 * f);
          }

          NewX3 += Scaled->Bpp;
        }
        break;
        // FIXME: ILfloat, ILdouble
    }
  }

  return Scaled;
}
