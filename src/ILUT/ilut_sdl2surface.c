//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2002 by Denton Woods
// Copyright (C) 2002 Nelson Rush.
// Last modified: 05/18/2002
//
// Filename: src-ILUT/src/ilut_sdlsurface.c
//
// Description:  SDL Surface functions for images
//
//-----------------------------------------------------------------------------


#include "ilut_internal.h"
#ifdef ILUT_USE_SDL2

#ifdef ILUT_USE_ALLEGRO
#undef main
#endif

#include <SDL2/SDL.h>

#ifdef  _MSC_VER
  #pragma comment(lib, "sdl2.lib")
#endif//_MSC_VER

static SDL_Surface * iConvertToSDL2Surface(ILimage *ilutCurImage, unsigned int flags)
{
  SDL_Surface *Bitmap = NULL;
  ILuint    i = 0, Pad, BppPal;
  ILubyte   *Dest = NULL, *Data = NULL;
  ILimage   *Image = NULL;
  ILboolean isBigEndian;
  ILuint rmask, gmask, bmask, amask;

  Image = ilutCurImage;
  if (ilutCurImage == NULL) {
    iSetError(ILUT_ILLEGAL_OPERATION);
    return NULL;
  }

#ifdef WORDS_BIGENDIAN
  isBigEndian = 1;
  rmask = 0xFF000000;
  gmask = 0x00FF0000;
  bmask = 0x0000FF00;
  amask = 0x000000FF;
#else
  isBigEndian = 0;
  rmask = 0x000000FF;
  gmask = 0x0000FF00;
  bmask = 0x00FF0000;
  amask = 0xFF000000;
#endif

  // Should be IL_BGR(A).
  if (ilutCurImage->Format == IL_RGB || ilutCurImage->Format == IL_RGBA) {
    if (!isBigEndian) {
      //iluSwapColours();  // No need to swap colors.  Just use the bitmasks.
      rmask = 0x00FF0000;
      gmask = 0x0000FF00;
      bmask = 0x000000FF;
    }
  }
  else if (ilutCurImage->Format == IL_BGR || ilutCurImage->Format == IL_BGRA) {
    if (isBigEndian) {
      rmask = 0x0000FF00;
      gmask = 0x00FF0000;
      bmask = 0xFF000000;
    }
  }
  else if (Image->Format != IL_COLOR_INDEX) {  // We have to convert the image.
    #ifdef WORDS_BIGENDIAN
    Image = iConvertImage(Image, IL_RGBA, IL_UNSIGNED_BYTE);
    #else
    Image = iConvertImage(Image, IL_BGRA, IL_UNSIGNED_BYTE);
    #endif
    if (Image == NULL)
      goto done;
  }

  if (Image->Type != IL_UNSIGNED_BYTE) {
    // We do not have to worry about Image != iCurImage at this point, because if it was converted,
    //  it was converted to a type of unsigned byte.
    Image = iConvertImage(Image, Image->Format, IL_UNSIGNED_BYTE);
    if (Image == NULL)
      goto done;
  }

  Data = Image->Data;
  if (Image->Origin == IL_ORIGIN_LOWER_LEFT) {
    Data = iGetFlipped(Image);
    if (Data == NULL)
      goto done;
  }

  Bitmap = SDL_CreateRGBSurface(flags, (int)Image->Width, (int)Image->Height, Image->Bpp * 8,
          rmask,gmask,bmask,amask);

  if (Bitmap == NULL)
    goto done;

  if (SDL_MUSTLOCK(Bitmap))
    SDL_LockSurface(Bitmap);

  Pad = Bitmap->pitch - Image->Bps;
  if (Pad == 0) {
    memcpy(Bitmap->pixels, Data, Image->SizeOfData);
  }
  else {  // Must pad the lines on some images.
    Dest = Bitmap->pixels;
    for (i = 0; i < Image->Height; i++) {
      memcpy(Dest, Data + i * Image->Bps, Image->Bps);
      imemclear(Dest + Image->Bps, Pad);
      Dest += Bitmap->pitch;
    }
  }

  if (SDL_MUSTLOCK(Bitmap))
    SDL_UnlockSurface(Bitmap);

  if (Image->Format == IL_COLOR_INDEX) {
    BppPal = iGetBppPal(Image->Pal.PalType);
    switch (ilutCurImage->Pal.PalType)
    {
      case IL_PAL_RGB24:
      case IL_PAL_RGB32:
        for (i = 0; i < ilutCurImage->Pal.PalSize / BppPal; i++) {
          (Bitmap->format)->palette->colors[i].r = ilutCurImage->Pal.Palette[i*BppPal+0];
          (Bitmap->format)->palette->colors[i].g = ilutCurImage->Pal.Palette[i*BppPal+1];
          (Bitmap->format)->palette->colors[i].b = ilutCurImage->Pal.Palette[i*BppPal+2];
          (Bitmap->format)->palette->colors[i].a = 0xFF;
        }
        break;

      case IL_PAL_RGBA32:
        for (i = 0; i < ilutCurImage->Pal.PalSize / BppPal; i++) {
          (Bitmap->format)->palette->colors[i].r = ilutCurImage->Pal.Palette[i*BppPal+0];
          (Bitmap->format)->palette->colors[i].g = ilutCurImage->Pal.Palette[i*BppPal+1];
          (Bitmap->format)->palette->colors[i].b = ilutCurImage->Pal.Palette[i*BppPal+2];
          (Bitmap->format)->palette->colors[i].a = ilutCurImage->Pal.Palette[i*BppPal+3];
        }
        break;

      case IL_PAL_BGR24:
      case IL_PAL_BGR32:
        for (i = 0; i < ilutCurImage->Pal.PalSize / BppPal; i++) {
          (Bitmap->format)->palette->colors[i].b = ilutCurImage->Pal.Palette[i*BppPal+0];
          (Bitmap->format)->palette->colors[i].g = ilutCurImage->Pal.Palette[i*BppPal+1];
          (Bitmap->format)->palette->colors[i].r = ilutCurImage->Pal.Palette[i*BppPal+2];
          (Bitmap->format)->palette->colors[i].a = 0xFF;
        }
        break;

      case IL_PAL_BGRA32:
        for (i = 0; i < ilutCurImage->Pal.PalSize / BppPal; i++) {
          (Bitmap->format)->palette->colors[i].b = ilutCurImage->Pal.Palette[i*BppPal+0];
          (Bitmap->format)->palette->colors[i].g = ilutCurImage->Pal.Palette[i*BppPal+1];
          (Bitmap->format)->palette->colors[i].r = ilutCurImage->Pal.Palette[i*BppPal+2];
          (Bitmap->format)->palette->colors[i].a = ilutCurImage->Pal.Palette[i*BppPal+3];
        }
        break;

      default:
        iSetError(IL_INTERNAL_ERROR);  // Do anything else?
    }
  }

done:
  if (Data != Image->Data)
    ifree(Data);  // This is flipped data.

  if (Image != ilutCurImage)
    iCloseImage(Image);  // This is a converted image.

  iUnlockImage(ilutCurImage);
  return Bitmap;  // This is NULL if there was an error.
}

// Does not account for converting luminance...
SDL_Surface *ILAPIENTRY ilutConvertToSDL2Surface(unsigned int flags)
{
  ILimage *ilutCurImage;
  SDL_Surface *Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  Result = iConvertToSDL2Surface(ilutCurImage, flags);
  iUnlockImage(ilutCurImage);
  return Result;
}

#ifndef _WIN32_WCE
SDL_Surface* ILAPIENTRY ilutSDLSurfaceLoadImage(ILstring FileName)
{
  SDL_Surface *Surface;
  ILimage * ilutCurImage, *Temp;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = iNewImage(1,1,1,1,1);
  Temp->io = ilutCurImage->io;
  iUnlockImage(ilutCurImage);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, FileName)) {
    return NULL;
  }

  Surface = iConvertToSDL2Surface(Temp, SDL_SWSURFACE);
  iCloseImage(Temp);

  return Surface;
}
#endif//_WIN32_WCE


// Unfinished
ILboolean ILAPIENTRY ilutSDL2SurfaceFromBitmap(SDL_Surface *Bitmap)
{
  ILimage * ilutCurImage;
  ILboolean Result;

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

  Result = iTexImage(ilutCurImage, (ILuint)Bitmap->w, (ILuint)Bitmap->h, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
  iUnlockImage(ilutCurImage);

  return Result;
}


#endif//ILUT_USE_SDL2
