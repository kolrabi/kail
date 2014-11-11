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

/**
 * Convert image into an allegro BITMAP.
 * @param  Image        Image to convert.
 * @param  Pal          Where to store palette information (if any).
 * @return              The allegro BITMAP.
 * @internal
 */
static BITMAP* iConvertToAlleg(ILimage *Image, PALETTE Pal) {
  BITMAP *Bitmap;
  ILimage *TempImage;
  ILuint i = 0, j = 0;

  if (Image == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return NULL;
  }

  TempImage = iCloneImage(Image);

  // Should be IL_BGR(A), but Djgpp screws up somewhere along the line.
  if (TempImage->Format == IL_RGB || TempImage->Format == IL_RGBA) {
    iSwapColours(TempImage);
  }

  if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
    iFlipImage(TempImage);
  if (TempImage->Type > IL_UNSIGNED_BYTE || TempImage->Type == IL_BYTE) 
    iConvertImages(TempImage, TempImage->Format, IL_UNSIGNED_BYTE);

  Bitmap = create_bitmap_ex(TempImage->Bpp * 8, (int)TempImage->Width, (int)TempImage->Height);
  if (Bitmap == NULL) {
    iCloseImage(TempImage);
    return NULL;
  }
  memcpy(Bitmap->dat, TempImage->Data, TempImage->SizeOfData);

  // Should we make this toggleable?
  if (TempImage->Format == IL_COLOUR_INDEX && TempImage->Pal.PalType != IL_PAL_NONE && Pal) {
    // Use the image's palette if there is one
    ILpal *TmpPal = iConvertPal(&TempImage->Pal, IL_PAL_RGB24);
    if (!TmpPal) {
      iCloseImage(TempImage);
      return NULL;
    }

    for (i=0; i < TmpPal->PalSize && i < 768; i += 3, j++) {
      Pal[j].r = TmpPal->Palette[i+0];
      Pal[j].g = TmpPal->Palette[i+1];
      Pal[j].b = TmpPal->Palette[i+2];
      Pal[j].filler = 255;
    }

    iClosePal(TmpPal);
    iCloseImage(TempImage);
  }

  return Bitmap;
}

/**
 * Convert the currently bound image to an Allegro BITMAP.
 * @param  Pal Where to store a possible colour palette.
 */
BITMAP* ILAPIENTRY ilutConvertToAlleg(PALETTE Pal) {
  iLockState();
  ILimage *Image = iLockCurImage();
  iUnlockState();

  BITMAP *Result = iConvertToAlleg(Image, Pal);
  iUnlockImage(Image);
  return Result;
}

#ifndef _WIN32_WCE
/**
 * Load an Allegro BITMAP from a file.
 * @param  FileName Name of file to load.
 * @return          The loaded BITMAP if sucessful, NULL otherwise.
 */
BITMAP* ILAPIENTRY ilutAllegLoadImage(ILconst_string FileName) {
  PALETTE Pal;
  ILimage *Image;
  BITMAP *Alleg;

  iLockState();
  Image = iNewImage(1,1,1,1,1);
  iUnlockState();

  if (!iLoad(Image, IL_TYPE_UNKNOWN, FileName)) {
    iCloseImage(Image);
    return NULL;
  }

  imemclear(&Pal, sizeof(Pal));
  Alleg = iConvertToAlleg(Image, Pal);
  iCloseImage(Image);
  return Alleg;
}
#endif//_WIN32_WCE


/**
 * Copy image data from an Allegro BITMAP into the currently bound image.
 * Unfinished.
 * @param  Bitmap BITMAP to use.
 * @return        Success of the operation.
 */
/*
ILboolean ILAPIENTRY ilutAllegFromBitmap(const BITMAP *Bitmap) {
  ILimage *Image;
  
  iLockState();
  Image = iLockCurImage();
  iUnlockState();

  if (Image == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Bitmap == NULL || Bitmap->w == 0 || Bitmap->h == 0) {
    iSetError(ILUT_INVALID_PARAM);
    iUnlockImage(Image);
    return IL_FALSE;
  }

  if (!iTexImage(Image, Bitmap->w, Bitmap->h, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL)) {
    iUnlockImage(Image);
    return IL_FALSE;
  }

  Image->Origin = IL_ORIGIN_LOWER_LEFT;  // I have no idea.

  // TODO

  iUnlockImage(Image);
  return IL_TRUE;
}
*/
#endif//ILUT_USE_ALLEGRO

