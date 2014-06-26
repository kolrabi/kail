//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000 by Denton Woods
// Last modified: 11/23/2000 <--Y2K Compliant! =]
//
// Filename: openilut/allegro.c
//
// Description:  Allegro functions for images
//
//-----------------------------------------------------------------------------

#include <IL/ilut_config.h>

#ifdef ILUT_USE_ALLEGRO
#include "ilut_allegro.h"
#include "ilut_internal.h" 

BITMAP* iConvertToAlleg(ILimage *ilutCurImage, PALETTE Pal) {
  BITMAP *Bitmap;
  ILimage *TempImage;
  ILuint i = 0, j = 0;

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return NULL;
  }

  // Should be IL_BGR(A), but Djgpp screws up somewhere along the line.
  if (ilutCurImage->Format == IL_RGB || ilutCurImage->Format == IL_RGBA) {
    iSwapColours(ilutCurImage);
  }

  if (ilutCurImage->Origin == IL_ORIGIN_LOWER_LEFT)
    iFlipImage(ilutCurImage);
  if (ilutCurImage->Type > IL_UNSIGNED_BYTE) {}  // Can't do anything about this right now...
  if (ilutCurImage->Type == IL_BYTE) {}  // Can't do anything about this right now...

  Bitmap = create_bitmap_ex(ilutCurImage->Bpp * 8, ilutCurImage->Width, ilutCurImage->Height);
  if (Bitmap == NULL) {
    return NULL;
  }
  memcpy(Bitmap->dat, ilutCurImage->Data, ilutCurImage->SizeOfData);

  // Should we make this toggleable?
  if (ilutCurImage->Bpp == 8 && ilutCurImage->Pal.PalType != IL_PAL_NONE) {
    // Use the image's palette if there is one
    // @TODO:  Use new ilCopyPal!!!
    TempImage = ilNewImage(ilutCurImage->Width, ilutCurImage->Height, ilutCurImage->Depth, ilutCurImage->Bpp, 1);
    ilCopyImageAttr(TempImage, ilutCurImage);

    if (!iConvertImagePal(TempImage, IL_PAL_RGB24)) {
      destroy_bitmap(Bitmap);
      iSetError(ILUT_ILLEGAL_OPERATION);
      return NULL;
    }

    for (; i < ilutCurImage->Pal.PalSize && i < 768; i += 3, j++) {
      Pal[j].r = TempImage->Pal.Palette[i+0];
      Pal[j].g = TempImage->Pal.Palette[i+1];
      Pal[j].b = TempImage->Pal.Palette[i+2];
      Pal[j].filler = 255;
    }

    ilCloseImage(TempImage);
  }

  return Bitmap;
}

// Does not account for converting luminance...
BITMAP* ILAPIENTRY ilutConvertToAlleg(PALETTE Pal) {
  iLockState();
  ILimage *Image = iLockCurImage();
  iUnlockState();

  BITMAP *Result = iConvertToAlleg(Image, Pal);
  iUnlockImage(Image);
  return Result;
}

#ifndef _WIN32_WCE
BITMAP* ILAPIENTRY ilutAllegLoadImage(ILstring FileName) {
  PALETTE Pal;
  ILimage *Image;
  BITMAP *Alleg;

  iLockState();
  Image = ilNewImage(1,1,1,1,1);
  iUnlockState();

  if (!iLoad(Image, IL_TYPE_UNKNOWN, FileName)) {
    ilCloseImage(Image);
    return NULL;
  }

  Alleg = iConvertToAlleg(Image, Pal);
  ilCloseImage(Image);
  return Alleg;
}
#endif//_WIN32_WCE


// Unfinished
ILboolean ILAPIENTRY ilutAllegFromBitmap(BITMAP *Bitmap) {
  ILimage *ilutCurImage;
  
  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Bitmap == NULL || Bitmap->w == 0 || Bitmap->h == 0) {
    iSetError(ILUT_INVALID_PARAM);
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }

  if (!iTexImage(ilutCurImage, Bitmap->w, Bitmap->h, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL)) {
    iUnlockImage(ilutCurImage);
    return IL_FALSE;
  }

  ilutCurImage->Origin = IL_ORIGIN_LOWER_LEFT;  // I have no idea.

  iUnlockImage(ilutCurImage);
  return IL_TRUE;
}

#endif//ILUT_USE_ALLEGRO

