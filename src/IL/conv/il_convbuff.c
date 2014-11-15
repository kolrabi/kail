//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 01/08/2007
//
// Filename: src-IL/src/il_convbuff.c
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "il_manip.h"
#include "il_pixel.h"
#include "il_convert.h"

#include <limits.h>

void* ILAPIENTRY iSwitchTypes(ILuint SizeOfData, ILenum SrcType, ILenum DestType, void *Buffer);


ILAPI void* ILAPIENTRY iConvertBuffer(ILuint SizeOfData, ILenum SrcFormat, ILenum DestFormat, ILenum SrcType, ILenum DestType, ILpal *SrcPal, void *Buffer)
{
  //static const  ILfloat LumFactor[3] = { 0.299f, 0.587f, 0.114f };  // Used for conversion to luminance
  //static const  ILfloat LumFactor[3] = { 0.3086f, 0.6094f, 0.0820f };  // http://www.sgi.com/grafica/matrix/index.html
  static const  ILfloat LumFactor[3] = { 0.212671f, 0.715160f, 0.072169f };  // http://www.inforamp.net/~poynton/ and libpng's libpng.txt

  void     *NewData = NULL;
  ILuint    i, j;
  ILuint    NumPix;  // Really number of pixels * bpp.
  ILuint    BpcDest, BppDest;
  void      *Data = NULL;
  ILimage   *TempImage = NULL;

  if (SizeOfData == 0 || Buffer == NULL) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }

  Data = iSwitchTypes(SizeOfData, SrcType, DestType, Buffer);
  if (Data == NULL)
    return NULL;

  BpcDest = iGetBpcType(DestType);
  BppDest = iGetBppFormat(DestFormat);
  NumPix  = SizeOfData / iGetBpcType(SrcType);

  if (DestFormat == SrcFormat) {
    NewData = (ILubyte*)ialloc(NumPix * BpcDest);
    if (NewData == NULL) {
      return NULL;
    }
    memcpy(NewData, Data, NumPix * BpcDest);
    if (Data != Buffer)
      ifree(Data);

    return NewData;
  }

  // Colour-indexed images are special here
  if (SrcFormat == IL_COLOUR_INDEX) {
    // We create a temporary palette image so that we can send it to iConvertPalette.
    ILimage   *PalImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
    if (PalImage == NULL)
      return NULL;

    // Populate the temporary palette image.
    PalImage->Pal.Palette = SrcPal->Palette;
    PalImage->Pal.PalSize = SrcPal->PalSize;
    PalImage->Pal.PalType = SrcPal->PalType;
    PalImage->Width       = NumPix;
    PalImage->Height      = 1;
    PalImage->Depth       = 1;
    PalImage->Format      = IL_COLOUR_INDEX;
    PalImage->Type        = IL_UNSIGNED_BYTE;
    PalImage->Data        = (ILubyte*)Buffer;
    PalImage->Bpp         = 1;
    PalImage->SizeOfData  = SizeOfData;

    // Convert the paletted image to a different format.
    TempImage = iConvertPalette(PalImage, DestFormat);
    if (TempImage == NULL) {
      // So that we do not delete the original palette or data.
      PalImage->Pal.Palette = NULL;
      PalImage->Data = NULL;
      iCloseImage(PalImage);
      return NULL;
    }

    // Set TempImage->Data to NULL so that we can preserve it via NewData, or
    //  else it would get wiped out by iCloseImage.
    NewData = TempImage->Data;
    TempImage->Data = NULL;

    // So that we do not delete the original palette or data.
    PalImage->Pal.Palette = NULL;
    PalImage->Data = NULL;

    Data = iSwitchTypes(TempImage->SizeOfData, IL_UNSIGNED_BYTE, DestType, NewData);

    // Clean up here.
    ifree(NewData);
    iCloseImage(PalImage);
    iCloseImage(TempImage);

    return Data;
  }

  NewData = ialloc(NumPix * BpcDest * BppDest);
  if (NewData == NULL) { 
    if (Data != Buffer) 
      ifree(Data); 
    return NULL; 
  }
  
  switch (SrcFormat)
  {
    case IL_RGB:
      switch (DestFormat)
      {
        case IL_BGR:
          for (i=0, j=0; i<NumPix; i+=3, j+=3) {
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
          }
          break;

        case IL_RGBA:
          for (i=0, j=0; i<NumPix; i+=3, j+=4) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+2, NewData, j+2, BpcDest );
            iSetPixelElementMax( NewData, j+3, DestType );
          }
          break;

        case IL_BGRA:
          for (i=0, j=0; i<NumPix; i+=3, j+=4) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iSetPixelElementMax( NewData, j+3, DestType );
          }
          break;

        case IL_LUMINANCE:
/*          iTrace("---- pixels: %f", iGetPixelElement( Data, 0, DestType ));
          iTrace("---- pixels: %f", iGetPixelElement( Data, 1, DestType ));
          iTrace("---- pixels: %f", iGetPixelElement( Data, 2, DestType ));*/
          for (i=0, j=0; i<NumPix; i+=3, j++) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[0];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[2];
            iSetPixelElement( NewData, j, DestType, IL_CLAMP(lum) );
            //if (i==0 || i == 3) iTrace("---- lum set %f", lum);
          }
//          iTrace("---- lum get %f", iGetPixelElement( NewData, 0, DestType ));
//          iTrace("---- lum get %f", iGetPixelElement( NewData, 1, DestType ));
          break;

        case IL_LUMINANCE_ALPHA:
          for (i=0, j=0; i<NumPix; i+=3, j+=2) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[0];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[2];
            iSetPixelElement( NewData, j+0, DestType, lum );
            iSetPixelElementMax( NewData, j+1, DestType );
          }
          break;

        case IL_ALPHA:
          for (i=0, j=0; i<NumPix; i+=3, j+=1) {
            iSetPixelElementMax( NewData, j, DestType );
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          ifree(NewData);
          return NULL;
      }
      break;

    case IL_RGBA:
      switch (DestFormat)
      {
        case IL_BGRA:
          for (i=0, j=0; i<NumPix; i+=4, j+=4) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iCopyPixelElement( Data, i+3, NewData, j+3, BpcDest );
          }
          break;

        case IL_RGB:
          for (i=0, j=0; i<NumPix; i+=4, j+=3) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+2, NewData, j+2, BpcDest );
          }
          break;

        case IL_BGR:
          for (i=0, j=0; i<NumPix; i+=4, j+=3) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
          }
          break;

        case IL_LUMINANCE:
          for (i=0, j=0; i<NumPix; i+=4, j++) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[0];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[2];
            iSetPixelElement( NewData, j, DestType, lum );
          }
          break;

        case IL_LUMINANCE_ALPHA:
          for (i=0, j=0; i<NumPix; i+=4, j+=2) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[0];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[2];
            iSetPixelElement( NewData, j, DestType, lum );
            iCopyPixelElement( Data, i+3, NewData, j+1, BpcDest );
          }
          break;

        case IL_ALPHA:
          for (i=0, j=0; i<NumPix; i+=4, j++) {
            iCopyPixelElement( Data, i+3, NewData, j, BpcDest );
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          ifree(NewData);
          return NULL;
      }
      break;

    case IL_BGR:
      switch (DestFormat)
      {
        case IL_RGB:
          for (i=0, j=0; i<NumPix; i+=3, j+=3) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
          }
          break;

        case IL_BGRA:
          for (i=0, j=0; i<NumPix; i+=3, j+=4) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+2, NewData, j+2, BpcDest );
            iSetPixelElementMax( NewData, j+3, DestType );
          }
          break;

        case IL_RGBA:
          for (i=0, j=0; i<NumPix; i+=3, j+=4) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iSetPixelElementMax( NewData, j+3, DestType );
          }
          break;

        case IL_LUMINANCE:
          for (i=0, j=0; i<NumPix; i+=3, j++) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[2];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[0];
            iSetPixelElement( NewData, j, DestType, lum );
          }
          break;

        case IL_LUMINANCE_ALPHA:
          for (i=0, j=0; i<NumPix; i+=3, j+=2) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[2];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[0];
            iSetPixelElement( NewData, j, DestType, lum );
            iSetPixelElementMax( NewData, j+1, DestType );
          }
          break;

        case IL_ALPHA:
          for (i=0, j=0; i<NumPix; i+=3, j+=1) {
            iSetPixelElementMax( NewData, j, DestType );
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;

    case IL_BGRA:
      switch (DestFormat)
      {
        case IL_RGBA:
          for (i=0, j=0; i<NumPix; i+=4, j+=4) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iCopyPixelElement( Data, i+3, NewData, j+3, BpcDest );
          }
          break;

        case IL_BGR:
          for (i=0, j=0; i<NumPix; i+=4, j+=3) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+2, NewData, j+2, BpcDest );
          }
          break;

        case IL_RGB:
          for (i=0, j=0; i<NumPix; i+=4, j+=3) {
            iCopyPixelElement( Data, i+2, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
          }
          break;

        case IL_LUMINANCE:
          for (i=0, j=0; i<NumPix; i+=4, j++) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[2];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[0];
            iSetPixelElement( NewData, j, DestType, lum );
          }
          break;

        case IL_LUMINANCE_ALPHA:
          for (i=0, j=0; i<NumPix; i+=4, j+=2) {
            ILfloat lum = 0.0f;
            lum += iGetPixelElement( Data, i+0, DestType ) * LumFactor[2];
            lum += iGetPixelElement( Data, i+1, DestType ) * LumFactor[1];
            lum += iGetPixelElement( Data, i+2, DestType ) * LumFactor[0];
            iSetPixelElement( NewData, j, DestType, lum );
            iCopyPixelElement( Data, i+3, NewData, j+1, BpcDest );
          }
          break;

        case IL_ALPHA:
          for (i=0, j=0; i<NumPix; i+=4, j++) {
            iCopyPixelElement( Data, i+3, NewData, j, BpcDest );
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          ifree(NewData);
          return NULL;
      }
      break;


    case IL_LUMINANCE:
      switch (DestFormat)
      {
        case IL_RGB:
        case IL_BGR:
          for (i=0, j=0; i<NumPix; i+=1, j+=3) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
          }
          break;

        case IL_RGBA:
        case IL_BGRA:
          for (i=0, j=0; i<NumPix; i+=1, j+=4) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iSetPixelElementMax( NewData, j+3, DestType );
          }
          break;

        case IL_LUMINANCE_ALPHA:
          for (i=0, j=0; i<NumPix; i+=1, j+=2) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iSetPixelElementMax( NewData, j+1, DestType );
          }
          break;

        case IL_ALPHA:
          for (i=0, j=0; i<NumPix; i+=1, j+=1) {
            iSetPixelElementMax( NewData, j, DestType );
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          ifree(NewData);
          return NULL;
      }
      break;


    case IL_LUMINANCE_ALPHA:
      switch (DestFormat)
      {
        case IL_RGB:
        case IL_BGR:
          for (i=0, j=0; i<NumPix; i+=2, j+=3) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
          }        
          break;

        case IL_RGBA:
        case IL_BGRA:
          for (i=0, j=0; i<NumPix; i+=2, j+=4) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iCopyPixelElement( Data, i+1, NewData, j+3, BpcDest );
          }        
          break;

        case IL_LUMINANCE:
          for (i=0, j=0; i<NumPix; i+=2, j+=1) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
          }        
          break;

        case IL_ALPHA:
          for (i=0, j=0; i<NumPix; i+=2, j+=1) {
            iCopyPixelElement( Data, i+1, NewData, j+0, BpcDest );
          }        
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;


    case IL_ALPHA:
      switch (DestFormat)
      {
        case IL_RGB:
        case IL_BGR:
          for (i=0, j=0; i<NumPix; i+=1, j+=3) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
          }
          break;

        case IL_RGBA:
        case IL_BGRA:
          for (i=0, j=0; i<NumPix; i+=1, j+=4) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+2, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+3, BpcDest );
          }        
          break;

        case IL_LUMINANCE:
          for (i=0, j=0; i<NumPix; i+=1, j+=1) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
          }        
          break;

        case IL_LUMINANCE_ALPHA:
          for (i=0, j=0; i<NumPix; i+=1, j+=2) {
            iCopyPixelElement( Data, i+0, NewData, j+0, BpcDest );
            iCopyPixelElement( Data, i+0, NewData, j+1, BpcDest );
          }        
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;
  }

  if (Data != Buffer)
    ifree(Data);

  return NewData;
}

// Really shouldn't have to check for default, as in above iConvertBuffer().
//  This now converts better from lower bpp to higher bpp.  For example, when
//  converting from 8 bpp to 16 bpp, if the value is 0xEC, the new value is 0xECEC
//  instead of 0xEC00.
void* ILAPIENTRY iSwitchTypes(ILuint SizeOfData, ILenum SrcType, ILenum DestType, void *Buffer)
{
  ILuint    BpcSrc, BpcDest, Size, i;
  void *    NewData;

  BpcSrc  = iGetBpcType(SrcType);
  BpcDest = iGetBpcType(DestType);

  if (BpcSrc == 0 || BpcDest == 0) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }

  if (SrcType == DestType) {
    return Buffer;
  }

  Size = SizeOfData / BpcSrc;

  NewData = ialloc(Size * BpcDest);
  if (NewData == NULL) {
    return NULL;
  }
  
  for (i = 0; i < Size; i++) {
    iSetPixelElement( NewData, i, DestType, iGetPixelElement( Buffer, i, SrcType ));
  }

  return NewData;
}




