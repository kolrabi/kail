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

#include <limits.h>


void* ILAPIENTRY iSwitchTypes(ILuint SizeOfData, ILenum SrcType, ILenum DestType, void *Buffer);

#define CHECK_ALLOC()   if (NewData == NULL) { \
              if (Data != Buffer) \
                ifree(Data); \
              return NULL; \
            }

/*
TODO:

static inline void ilConvByteToByte   (const ILubyte *in, ILubyte  *out)  { out =                                                     in[0]; }
static inline void ilConvByteToShort  (const ILubyte *in, ILushort *out)  { out =                                    ( in[0] <<  8) | in[0]; }
static inline void ilConvByteToInt    (const ILubyte *in, ILuint   *out)  { out = ( in[0] << 24) | ( in[0] << 16 ) | ( in[0] <<  8) | in[0]; }
static inline void ilConvByteToFloat  (const ILubyte *in, ILfloat  *out)  { out =   in[0] / (float)UCHAR_MAX;                                }
static inline void ilConvByteToDouble (const ILubyte *in, ILdouble *out)  { out =   in[0] / (double)UCHAR_MAX;                               }
static inline void ilConvByteToHalf   (const ILubyte *in, void     *out)  { float val = in[0] / (float)UCHAR_MAX; ilFloatToHalfV(&val, out); }

*/

ILAPI void* ILAPIENTRY iConvertBuffer(ILuint SizeOfData, ILenum SrcFormat, ILenum DestFormat, ILenum SrcType, ILenum DestType, ILpal *SrcPal, void *Buffer)
{
  //static const  ILfloat LumFactor[3] = { 0.299f, 0.587f, 0.114f };  // Used for conversion to luminance
  //static const  ILfloat LumFactor[3] = { 0.3086f, 0.6094f, 0.0820f };  // http://www.sgi.com/grafica/matrix/index.html
  static const  ILfloat LumFactor[3] = { 0.212671f, 0.715160f, 0.072169f };  // http://www.inforamp.net/~poynton/ and libpng's libpng.txt

  void     *NewData = NULL;
  ILuint    i, j, c, Size;
  ILfloat   Resultf;
  ILdouble  Resultd;
  ILuint    NumPix;  // Really number of pixels * bpp.
  ILuint    BpcDest;
  void      *Data = NULL;
  ILimage   *PalImage = NULL, *TempImage = NULL;

  if (SizeOfData == 0 || Buffer == NULL) {
    iSetError(IL_INVALID_PARAM);
    return NULL;
  }

  Data = iSwitchTypes(SizeOfData, SrcType, DestType, Buffer);
  if (Data == NULL)
    return NULL;

  BpcDest = iGetBpcType(DestType);
  NumPix = SizeOfData / iGetBpcType(SrcType);

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
    PalImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
    if (PalImage == NULL)
      return NULL;
    // Populate the temporary palette image.
    PalImage->Pal.Palette = SrcPal->Palette;
    PalImage->Pal.PalSize = SrcPal->PalSize;
    PalImage->Pal.PalType = SrcPal->PalType;
    PalImage->Width = NumPix;
    PalImage->Height = 1;
    PalImage->Depth = 1;
    PalImage->Format = IL_COLOUR_INDEX;
    PalImage->Type = IL_UNSIGNED_BYTE;
    PalImage->Data = (ILubyte*)Buffer;
    PalImage->Bpp = 1;
    PalImage->SizeOfData = SizeOfData;

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

    // Clean up here.
    iCloseImage(PalImage);
    iCloseImage(TempImage);
    return NewData;
  }
  
  switch (SrcFormat)
  {
    case IL_RGB:
      switch (DestFormat)
      {
        case IL_BGR:
          NewData = ialloc(NumPix * BpcDest);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < NumPix; i += 3) {
                ((ILubyte*)(NewData))[i  ] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[i+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[i+2] = ((ILubyte*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < NumPix; i += 3) {
                ((ILushort*)(NewData))[i  ] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[i+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[i+2] = ((ILushort*)(Data))[i  ];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < NumPix; i += 3) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[i+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[i+2] = ((ILuint*)(Data))[i];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < NumPix; i += 3) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[i+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[i+2] = ((ILfloat*)(Data))[i];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < NumPix; i += 3) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[i+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[i+2] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;

        case IL_RGBA:
          NewData = ialloc(NumPix * BpcDest * 4 / 3);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[j+3] = UCHAR_MAX;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[j+3] = USHRT_MAX;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[j+3] = UINT_MAX;
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[j+3] = 1.0f;
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[j+3] = 1.0;
              }
              break;
          }
          break;

        case IL_BGRA:
          NewData = ialloc(NumPix * BpcDest * 4 / 3);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[j+3] = UCHAR_MAX;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[j+3] = USHRT_MAX;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[j+3] = UINT_MAX;
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[j+3] = 1.0f;
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[j+3] = 1.0f;
              }
              break;
          }
          break;

        case IL_LUMINANCE:
          NewData = ialloc(NumPix * BpcDest / 3);
          CHECK_ALLOC();
          Size = NumPix / 3;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILubyte*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILubyte*)(NewData))[i] = (ILubyte)Resultf;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILushort*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILushort*)(NewData))[i] = (ILushort)Resultf;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILuint*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILuint*)(NewData))[i] = (ILuint)Resultf;
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILfloat*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILfloat*)(NewData))[i] = Resultf;
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                Resultd = 0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILdouble*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILdouble*)(NewData))[i] = Resultd;
              }
              break;
          }
          break;

        case IL_LUMINANCE_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 3 * 2);
          CHECK_ALLOC();
          Size = NumPix / 3;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILubyte*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILubyte*)(NewData))[i*2  ] = (ILubyte)Resultf;
                ((ILubyte*)(NewData))[i*2+1] = UCHAR_MAX;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILushort*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILushort*)(NewData))[i*2] = (ILushort)Resultf;
                ((ILushort*)(NewData))[i*2+1] = USHRT_MAX;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILuint*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILuint*)(NewData))[i*2] = (ILuint)Resultf;
                ((ILuint*)(NewData))[i*2+1] = UINT_MAX;
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILfloat*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILfloat*)(NewData))[i*2] = Resultf;
                ((ILfloat*)(NewData))[i*2+1] = 1.0f;
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                Resultd = 0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILdouble*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILdouble*)(NewData))[i*2] = Resultd;
                ((ILdouble*)(NewData))[i*2+1] = 1.0;
              }
              break;
          }
          break;

        case IL_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 3);
          CHECK_ALLOC();
          memset(NewData, 0, NumPix * BpcDest);
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;

    case IL_RGBA:
      switch (DestFormat)
      {
        case IL_BGRA:
          NewData = ialloc(NumPix * BpcDest);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < NumPix; i += 4) {
                ((ILubyte*)(NewData))[i  ] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[i+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[i+2] = ((ILubyte*)(Data))[i  ];
                ((ILubyte*)(NewData))[i+3] = ((ILubyte*)(Data))[i+3];
              }
              break;

            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < NumPix; i += 4) {
                ((ILushort*)(NewData))[i] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[i+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[i+2] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[i+3] = ((ILushort*)(Data))[i+3];
              }
              break;

            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < NumPix; i += 4) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[i+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[i+2] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[i+3] = ((ILuint*)(Data))[i+3];
              }
              break;

            case IL_FLOAT:
              for (i = 0; i < NumPix; i += 4) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[i+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[i+2] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[i+3] = ((ILfloat*)(Data))[i+3];
              }
              break;

            case IL_DOUBLE:
              for (i = 0; i < NumPix; i += 4) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[i+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[i+2] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[i+3] = ((ILdouble*)(Data))[i+3];
              }
              break;
          }
          break;

        case IL_RGB:
          NewData = ialloc(NumPix * BpcDest * 3 / 4);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILubyte*)(NewData))[j  ] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i+2];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i+2];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i+2];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i+2];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i+2];
              }
              break;
          }
          break;

        case IL_BGR:
          NewData = ialloc(NumPix * BpcDest * 3 / 4);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILubyte*)(NewData))[j  ] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;

        case IL_LUMINANCE:
          NewData = ialloc(NumPix * BpcDest / 4);
          CHECK_ALLOC();
          Size = NumPix / 4;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                Resultf = 0.0f;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILubyte*)(Data))[i * 4 + c] * LumFactor[c];
                }
                ((ILubyte*)(NewData))[i] = (ILubyte)Resultf;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                Resultf = 0.0f;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILushort*)(Data))[i * 4 + c] * LumFactor[c];
                }
                ((ILushort*)(NewData))[i] = (ILushort)Resultf;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILuint*)(Data))[i * 4 + c] * LumFactor[c];
                }
                ((ILuint*)(NewData))[i] = (ILuint)Resultd;
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILfloat*)(Data))[i * 4 + c] * LumFactor[c];
                }
                ((ILfloat*)(NewData))[i] = (ILfloat)Resultd;
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILdouble*)(Data))[i * 4 + c] * LumFactor[c];
                }
                ((ILdouble*)(NewData))[i] = Resultd;
              }
              break;
          }
          break;

        case IL_LUMINANCE_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 4 * 2);
          CHECK_ALLOC();
          Size = NumPix / 4 * 2;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i += 2) {
                Resultf = 0.0f;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILubyte*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILubyte*)(NewData))[i] = (ILubyte)Resultf;
                ((ILubyte*)(NewData))[i+1] = ((ILubyte*)(Data))[i * 2 + 3];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i += 2) {
                Resultf = 0.0f;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILushort*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILushort*)(NewData))[i] = (ILushort)Resultf;
                ((ILushort*)(NewData))[i+1] = ((ILushort*)(Data))[i * 2 + 3];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i += 2) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILuint*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILuint*)(NewData))[i] = (ILuint)Resultd;
                ((ILuint*)(NewData))[i+1] = ((ILuint*)(Data))[i * 2 + 3];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i += 2) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILfloat*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILfloat*)(NewData))[i] = (ILfloat)Resultd;
                ((ILfloat*)(NewData))[i+1] = ((ILfloat*)(Data))[i * 2 + 3];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i += 2) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILdouble*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILdouble*)(NewData))[i] = Resultd;
                ((ILdouble*)(NewData))[i+1] = ((ILdouble*)(Data))[i * 2 + 3];
              }
              break;
          }
          break;

        case IL_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 4);
          CHECK_ALLOC();
          Size = NumPix / 4;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                ((ILubyte*)(NewData))[i] = ((ILubyte*)(Data))[i * 4 + 3];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                ((ILushort*)(NewData))[i] = ((ILushort*)(Data))[i * 4 + 3];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i * 4 + 3];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i * 4 + 3];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i * 4 + 3];
              }
              break;
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;

    case IL_BGR:
      switch (DestFormat)
      {
        case IL_RGB:
          NewData = ialloc(NumPix * BpcDest);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < NumPix; i += 3) {
                ((ILubyte*)(NewData))[i] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[i+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[i+2] = ((ILubyte*)(Data))[i];
              }
              break;

            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < NumPix; i += 3) {
                ((ILushort*)(NewData))[i] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[i+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[i+2] = ((ILushort*)(Data))[i];
              }
              break;

            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < NumPix; i += 3) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[i+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[i+2] = ((ILuint*)(Data))[i];
              }
              break;

            case IL_FLOAT:
              for (i = 0; i < NumPix; i += 3) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[i+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[i+2] = ((ILfloat*)(Data))[i];
              }
              break;

            case IL_DOUBLE:
              for (i = 0; i < NumPix; i += 3) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[i+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[i+2] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;

        case IL_BGRA:
          NewData = ialloc(NumPix * BpcDest * 4 / 3);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[j+3] = UCHAR_MAX;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[j+3] = USHRT_MAX;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[j+3] = UINT_MAX;
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[j+3] = 1.0f;
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[j+3] = 1.0;
              }
              break;
          }
          break;

        case IL_RGBA:
          NewData = ialloc(NumPix * BpcDest * 4 / 3);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[j+3] = UCHAR_MAX;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[j+3] = USHRT_MAX;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[j+3] = UINT_MAX;
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[j+3] = 1.0f;
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 3, j += 4) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[j+3] = 1.0;
              }
              break;
          }
          break;

        case IL_LUMINANCE:
          NewData = ialloc(NumPix * BpcDest / 3);
          CHECK_ALLOC();
          Size = NumPix / 3;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                Resultf = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultf += ((ILubyte*)(Data))[i * 3 + c] * LumFactor[j];
                }
                ((ILubyte*)(NewData))[i] = (ILubyte)Resultf;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                Resultf = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultf += ((ILushort*)(Data))[i * 3 + c] * LumFactor[j];
                }
                ((ILushort*)(NewData))[i] = (ILushort)Resultf;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultd += ((ILuint*)(Data))[i * 3 + c] * LumFactor[j];
                }
                ((ILuint*)(NewData))[i] = (ILuint)Resultd;
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultd += ((ILfloat*)(Data))[i * 3 + c] * LumFactor[j];
                }
                ((ILfloat*)(NewData))[i] = (ILfloat)Resultd;
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultd += ((ILdouble*)(Data))[i * 3 + c] * LumFactor[j];
                }
                ((ILdouble*)(NewData))[i] = Resultd;
              }
              break;
          }
          break;

        case IL_LUMINANCE_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 3 * 2);
          CHECK_ALLOC();
          Size = NumPix / 3;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILubyte*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILubyte*)(NewData))[i*2] = (ILubyte)Resultf;
                ((ILubyte*)(NewData))[i*2+1] = UCHAR_MAX;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILushort*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILushort*)(NewData))[i*2] = (ILushort)Resultf;
                ((ILushort*)(NewData))[i*2+1] = USHRT_MAX;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILuint*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILuint*)(NewData))[i*2] = (ILuint)Resultf;
                ((ILuint*)(NewData))[i*2+1] = UINT_MAX;
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                Resultf = 0;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILfloat*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILfloat*)(NewData))[i*2] = Resultf;
                ((ILfloat*)(NewData))[i*2+1] = 1.0f;
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                Resultd = 0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILdouble*)(Data))[i * 3 + c] * LumFactor[c];
                }
                ((ILdouble*)(NewData))[i*2] = Resultd;
                ((ILdouble*)(NewData))[i*2+1] = 1.0;
              }
              break;
          }
          break;

        case IL_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 3);
          CHECK_ALLOC();
          memset(NewData, 0, NumPix * BpcDest / 3);
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
          NewData = ialloc(NumPix * BpcDest);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < NumPix; i += 4) {
                ((ILubyte*)(NewData))[i] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[i+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[i+2] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[i+3] = ((ILubyte*)(Data))[i+3];
              }
              break;

            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < NumPix; i += 4) {
                ((ILushort*)(NewData))[i] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[i+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[i+2] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[i+3] = ((ILushort*)(Data))[i+3];
              }
              break;

            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < NumPix; i += 4) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[i+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[i+2] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[i+3] = ((ILuint*)(Data))[i+3];
              }
              break;

            case IL_FLOAT:
              for (i = 0; i < NumPix; i += 4) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[i+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[i+2] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[i+3] = ((ILfloat*)(Data))[i+3];
              }
              break;

            case IL_DOUBLE:
              for (i = 0; i < NumPix; i += 4) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[i+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[i+2] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[i+3] = ((ILdouble*)(Data))[i+3];
              }
              break;
          }
          break;

        case IL_BGR:
          NewData = ialloc(NumPix * BpcDest * 3 / 4);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i+2];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i+2];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i+2];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i+2];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i+2];
              }
              break;
          }
          break;

        case IL_RGB:
          NewData = ialloc(NumPix * BpcDest * 3 / 4);
          CHECK_ALLOC();
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i+2];
                ((ILubyte*)(NewData))[j+1] = ((ILubyte*)(Data))[i+1];
                ((ILubyte*)(NewData))[j+2] = ((ILubyte*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i+2];
                ((ILushort*)(NewData))[j+1] = ((ILushort*)(Data))[i+1];
                ((ILushort*)(NewData))[j+2] = ((ILushort*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i+2];
                ((ILuint*)(NewData))[j+1] = ((ILuint*)(Data))[i+1];
                ((ILuint*)(NewData))[j+2] = ((ILuint*)(Data))[i];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i+2];
                ((ILfloat*)(NewData))[j+1] = ((ILfloat*)(Data))[i+1];
                ((ILfloat*)(NewData))[j+2] = ((ILfloat*)(Data))[i];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 4, j += 3) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i+2];
                ((ILdouble*)(NewData))[j+1] = ((ILdouble*)(Data))[i+1];
                ((ILdouble*)(NewData))[j+2] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;

        case IL_LUMINANCE:
          NewData = ialloc(NumPix * BpcDest / 4);
          CHECK_ALLOC();
          Size = NumPix / 4;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                Resultf = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultf += ((ILubyte*)(Data))[i * 4 + c] * LumFactor[j];
                }
                ((ILubyte*)(NewData))[i] = (ILubyte)Resultf;
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                Resultf = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultf += ((ILushort*)(Data))[i * 4 + c] * LumFactor[j];
                }
                ((ILushort*)(NewData))[i] = (ILushort)Resultf;
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultd += ((ILuint*)(Data))[i * 4 + c] * LumFactor[j];
                }
                ((ILuint*)(NewData))[i] = (ILuint)Resultd;
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultd += ((ILfloat*)(Data))[i * 4 + c] * LumFactor[j];
                }
                ((ILfloat*)(NewData))[i] = (ILfloat)Resultd;
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                Resultd = 0.0f;  j = 2;
                for (c = 0; c < 3; c++, j--) {
                  Resultd += ((ILdouble*)(Data))[i * 4 + c] * LumFactor[j];
                }
                ((ILdouble*)(NewData))[i] = Resultd;
              }
              break;
          }
          break;

        case IL_LUMINANCE_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 4 * 2);
          CHECK_ALLOC();
          Size = NumPix / 4 * 2;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i += 2) {
                Resultf = 0.0f;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILubyte*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILubyte*)(NewData))[i] = (ILubyte)Resultf;
                ((ILubyte*)(NewData))[i+1] = ((ILubyte*)(Data))[i * 2 + 3];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i += 2) {
                Resultf = 0.0f;
                for (c = 0; c < 3; c++) {
                  Resultf += ((ILushort*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILushort*)(NewData))[i] = (ILushort)Resultf;
                ((ILushort*)(NewData))[i+1] = ((ILushort*)(Data))[i * 2 + 3];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i += 2) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILuint*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILuint*)(NewData))[i] = (ILuint)Resultd;
                ((ILuint*)(NewData))[i+1] = ((ILuint*)(Data))[i * 2 + 3];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i += 2) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILfloat*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILfloat*)(NewData))[i] = (ILfloat)Resultd;
                ((ILfloat*)(NewData))[i+1] = ((ILfloat*)(Data))[i * 2 + 3];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i += 2) {
                Resultd = 0.0;
                for (c = 0; c < 3; c++) {
                  Resultd += ((ILdouble*)(Data))[i * 2 + c] * LumFactor[c];
                }
                ((ILdouble*)(NewData))[i] = Resultd;
                ((ILdouble*)(NewData))[i+1] = ((ILdouble*)(Data))[i * 2 + 3];
              }
              break;
          }
          break;

        case IL_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 4);
          CHECK_ALLOC();
          Size = NumPix / 4;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                ((ILubyte*)(NewData))[i] = ((ILubyte*)(Data))[i * 4 + 3];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                ((ILushort*)(NewData))[i] = ((ILushort*)(Data))[i * 4 + 3];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i * 4 + 3];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i * 4 + 3];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i * 4 + 3];
              }
              break;
          }
          break;

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;


    case IL_LUMINANCE:
      switch (DestFormat)
      {
        case IL_RGB:
        case IL_BGR:
          NewData = ialloc(NumPix * BpcDest * 3);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i++, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILubyte*)(NewData))[j + c] = ((ILubyte*)(Data))[i];
                }
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i++, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILushort*)(NewData))[j + c] = ((ILushort*)(Data))[i];
                }
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i++, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILuint*)(NewData))[j + c] = ((ILuint*)(Data))[i];
                }
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i++, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILfloat*)(NewData))[j + c] = ((ILfloat*)(Data))[i];
                }
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i++, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILdouble*)(NewData))[j + c] = ((ILdouble*)(Data))[i];
                }
              }
              break;
          }
          break;

        case IL_RGBA:
        case IL_BGRA:
          NewData = ialloc(NumPix * BpcDest * 4);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILubyte*)(NewData))[j + c] = ((ILubyte*)(Data))[i];
                }
                ((ILubyte*)(NewData))[j + 3] = UCHAR_MAX;  // Full opacity
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILushort*)(NewData))[j + c] = ((ILushort*)(Data))[i];
                }
                ((ILushort*)(NewData))[j + 3] = USHRT_MAX;  // Full opacity
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILuint*)(NewData))[j + c] = ((ILuint*)(Data))[i];
                }
                ((ILuint*)(NewData))[j + 3] = UINT_MAX;  // Full opacity
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILfloat*)(NewData))[j + c] = ((ILfloat*)(Data))[i];
                }
                ((ILfloat*)(NewData))[j + 3] = 1.0f;  // Full opacity
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILdouble*)(NewData))[j + c] = ((ILdouble*)(Data))[i];
                }
                ((ILdouble*)(NewData))[j + 3] = 1.0;  // Full opacity
              }
              break;
          }
          break;

        case IL_LUMINANCE_ALPHA:
          NewData = ialloc(NumPix * BpcDest * 2);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < NumPix; i++) {
                ((ILubyte*)(NewData))[i * 2] = ((ILubyte*)(Data))[i];
                ((ILubyte*)(NewData))[i * 2 + 1] = UCHAR_MAX;  // Full opacity
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < NumPix; i++) {
                ((ILushort*)(NewData))[i * 2] = ((ILushort*)(Data))[i];
                ((ILushort*)(NewData))[i * 2 + 1] = USHRT_MAX;  // Full opacity
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < NumPix; i++) {
                ((ILuint*)(NewData))[i * 2] = ((ILuint*)(Data))[i];
                ((ILuint*)(NewData))[i * 2 + 1] = UINT_MAX;  // Full opacity
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < NumPix; i++) {
                ((ILfloat*)(NewData))[i * 2] = ((ILfloat*)(Data))[i];
                ((ILfloat*)(NewData))[i * 2 + 1] = 1.0f;  // Full opacity
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < NumPix; i++) {
                ((ILdouble*)(NewData))[i * 2] = ((ILdouble*)(Data))[i];
                ((ILdouble*)(NewData))[i * 2 + 1] = 1.0;  // Full opacity
              }
              break;
          }
          break;

        case IL_ALPHA:
          NewData = ialloc(NumPix * BpcDest);
          CHECK_ALLOC();
          memset(NewData, 0, NumPix * BpcDest);
          break;

        /*case IL_COLOUR_INDEX:
          NewData = (ILubyte*)ialloc(Image->SizeOfData);
          NewImage->Pal.Palette = (ILubyte*)ialloc(768);
          if (NewData == NULL || NewImage->Pal.Palette) {
            ifree(NewImage);
            return IL_FALSE;
          }

          // Fill the palette
          for (i = 0; i < 256; i++) {
            for (c = 0; c < 3; c++) {
              NewImage->Pal.Palette[i * 3 + c] = (ILubyte)i;
            }
          }
          // Copy the data
          for (i = 0; i < Image->SizeOfData; i++) {
            NewData[i] = Image->Data[i];
          }
          break;*/

        default:
          iSetError(IL_INVALID_CONVERSION);
          if (Data != Buffer)
            ifree(Data);
          return NULL;
      }
      break;


    case IL_LUMINANCE_ALPHA:
      switch (DestFormat)
      {
        case IL_RGB:
        case IL_BGR:
          NewData = ialloc(NumPix * BpcDest / 2 * 3);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILubyte*)(NewData))[j + c] = ((ILubyte*)(Data))[i];
                }
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILushort*)(NewData))[j + c] = ((ILushort*)(Data))[i];
                }
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILuint*)(NewData))[j + c] = ((ILuint*)(Data))[i];
                }
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILfloat*)(NewData))[j + c] = ((ILfloat*)(Data))[i];
                }
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 3) {
                for (c = 0; c < 3; c++) {
                  ((ILdouble*)(NewData))[j + c] = ((ILdouble*)(Data))[i];
                }
              }
              break;
          }
          break;

        case IL_RGBA:
        case IL_BGRA:
          NewData = ialloc(NumPix * BpcDest / 2 * 4);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILubyte*)(NewData))[j + c] = ((ILubyte*)(Data))[i];
                }
                ((ILubyte*)(NewData))[j + 3] = ((ILubyte*)(Data))[i+1];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILushort*)(NewData))[j + c] = ((ILushort*)(Data))[i];
                }
                ((ILushort*)(NewData))[j + 3] = ((ILushort*)(Data))[i+1];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILuint*)(NewData))[j + c] = ((ILuint*)(Data))[i];
                }
                ((ILuint*)(NewData))[j + 3] = ((ILuint*)(Data))[i+1];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILfloat*)(NewData))[j + c] = ((ILfloat*)(Data))[i];
                }
                ((ILfloat*)(NewData))[j + 3] = ((ILfloat*)(Data))[i+1];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 2, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILdouble*)(NewData))[j + c] = ((ILdouble*)(Data))[i];
                }
                ((ILdouble*)(NewData))[j + 3] = ((ILdouble*)(Data))[i+1];
              }
              break;
          }
          break;

        case IL_LUMINANCE:
          NewData = ialloc(NumPix * BpcDest / 2);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i += 2, j++) {
                ((ILubyte*)(NewData))[j] = ((ILubyte*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i += 2, j++) {
                ((ILushort*)(NewData))[j] = ((ILushort*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i += 2, j++) {
                ((ILuint*)(NewData))[j] = ((ILuint*)(Data))[i];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i += 2, j++) {
                ((ILfloat*)(NewData))[j] = ((ILfloat*)(Data))[i];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i += 2, j++) {
                ((ILdouble*)(NewData))[j] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;

        case IL_ALPHA:
          NewData = ialloc(NumPix * BpcDest / 2);
          CHECK_ALLOC();
          Size = NumPix / 2;
          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < Size; i++) {
                ((ILubyte*)(NewData))[i] = ((ILubyte*)(Data))[i * 2 + 3];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < Size; i++) {
                ((ILushort*)(NewData))[i] = ((ILushort*)(Data))[i * 2 + 3];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < Size; i++) {
                ((ILuint*)(NewData))[i] = ((ILuint*)(Data))[i * 2 + 3];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < Size; i++) {
                ((ILfloat*)(NewData))[i] = ((ILfloat*)(Data))[i * 2 + 3];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < Size; i++) {
                ((ILdouble*)(NewData))[i] = ((ILdouble*)(Data))[i * 2 + 3];
              }
              break;
          }
          break;

        /*case IL_COLOUR_INDEX:
          NewData = (ILubyte*)ialloc(Image->SizeOfData);
          NewImage->Pal.Palette = (ILubyte*)ialloc(768);
          if (NewData == NULL || NewImage->Pal.Palette) {
            ifree(NewImage);
            return IL_FALSE;
          }

          // Fill the palette
          for (i = 0; i < 256; i++) {
            for (c = 0; c < 3; c++) {
              NewImage->Pal.Palette[i * 3 + c] = (ILubyte)i;
            }
          }
          // Copy the data
          for (i = 0; i < Image->SizeOfData; i++) {
            NewData[i] = Image->Data[i];
          }
          break;*/

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
          NewData = ialloc(NumPix * BpcDest * 3);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
            case IL_UNSIGNED_INT:
            case IL_INT:
            case IL_FLOAT:
            case IL_DOUBLE:
              memset(NewData, 0, NumPix * BpcDest * 3);  // Easy enough
              break;
            //@TODO: Do we need to a check for default: (error)?
          }
          break;

        case IL_RGBA:
        case IL_BGRA:
          NewData = ialloc(NumPix * BpcDest * 4);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILubyte*)(NewData))[j + c] = 0;
                }
                ((ILubyte*)(NewData))[j + 3] = ((ILubyte*)(Data))[i];  // Only value that matters
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILushort*)(NewData))[j + c] = 0;
                }
                ((ILushort*)(NewData))[j + 3] = ((ILushort*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILuint*)(NewData))[j + c] = 0;
                }
                ((ILuint*)(NewData))[j + 3] = ((ILuint*)(Data))[i];
              }
              break;
            case IL_FLOAT:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILfloat*)(NewData))[j + c] = 0;
                }
                ((ILfloat*)(NewData))[j + 3] = ((ILfloat*)(Data))[i];
              }
              break;
            case IL_DOUBLE:
              for (i = 0, j = 0; i < NumPix; i++, j += 4) {
                for (c = 0; c < 3; c++) {
                  ((ILdouble*)(NewData))[j + c] = 0;
                }
                ((ILdouble*)(NewData))[j + 3] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;

        case IL_LUMINANCE_ALPHA:
          NewData = ialloc(NumPix * BpcDest * 2);
          CHECK_ALLOC();

          switch (DestType)
          {
            case IL_UNSIGNED_BYTE:
            case IL_BYTE:
              for (i = 0; i < NumPix; i++) {
                ((ILubyte*)(NewData))[i * 2] = 0;
                ((ILubyte*)(NewData))[i * 2 + 1] = ((ILubyte*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_SHORT:
            case IL_SHORT:
            case IL_HALF:
              for (i = 0; i < NumPix; i++) {
                ((ILushort*)(NewData))[i * 2] = 0;
                ((ILushort*)(NewData))[i * 2 + 1] = ((ILushort*)(Data))[i];
              }
              break;
            case IL_UNSIGNED_INT:
            case IL_INT:
              for (i = 0; i < NumPix; i++) {
                ((ILuint*)(NewData))[i * 2] = 0;
                ((ILuint*)(NewData))[i * 2 + 1] = ((ILuint*)(Data))[i];
              }
              break;
            case IL_FLOAT:
              for (i = 0; i < NumPix; i++) {
                ((ILfloat*)(NewData))[i * 2] = 0;
                ((ILfloat*)(NewData))[i * 2 + 1] = ((ILfloat*)(Data))[i];
              }
              break;
            case IL_DOUBLE:
              for (i = 0; i < NumPix; i++) {
                ((ILdouble*)(NewData))[i * 2] = 0;
                ((ILdouble*)(NewData))[i * 2 + 1] = ((ILdouble*)(Data))[i];
              }
              break;
          }
          break;


        /*case IL_COLOUR_INDEX:
          NewData = (ILubyte*)ialloc(Image->SizeOfData);
          NewImage->Pal.Palette = (ILubyte*)ialloc(768);
          if (NewData == NULL || NewImage->Pal.Palette) {
            ifree(NewImage);
            return IL_FALSE;
          }

          // Fill the palette
          for (i = 0; i < 256; i++) {
            for (c = 0; c < 3; c++) {
              NewImage->Pal.Palette[i * 3 + c] = (ILubyte)i;
            }
          }
          // Copy the data
          for (i = 0; i < Image->SizeOfData; i++) {
            NewData[i] = Image->Data[i];
          }
          break;*/

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
  ILubyte   *BytePtr;
  ILushort  *ShortPtr;
  ILuint    *IntPtr;
  ILfloat   *FloatPtr, tempFloat;
  ILdouble  *DblPtr, tempDouble;
  ILushort  *HalfPtr;
  void      *NewData;

  ILubyte   *ByteBuf = (ILubyte*)Buffer;

  BpcSrc  = iGetBpcType(SrcType);
  BpcDest = iGetBpcType(DestType);

  if (BpcSrc == 0 || BpcDest == 0) {
    iSetError(IL_INTERNAL_ERROR);
    return NULL;
  }

  Size = SizeOfData / BpcSrc;

  //if (BpcSrc == BpcDest) {
  if (SrcType == DestType) {
    return Buffer;
  }

  NewData = ialloc(Size * BpcDest);
  if (NewData == NULL) {
    return NULL;
  }

  switch (DestType)
  {
    case IL_UNSIGNED_BYTE:
    case IL_BYTE:
      BytePtr = (ILubyte*)NewData;
      switch (SrcType)
      {
        case IL_UNSIGNED_SHORT:
        case IL_SHORT:
          for (i = 0; i < Size; i++) {
            BytePtr[i] = ((ILushort*)Buffer)[i] >> 8;
          }
          break;
        case IL_UNSIGNED_INT:
        case IL_INT:
          for (i = 0; i < Size; i++) {
            BytePtr[i] = ((ILuint*)Buffer)[i] >> 24;
          }
          break;
        case IL_FLOAT:
          for (i = 0; i < Size; i++) {
            #if CLAMP_FLOATS
              tempFloat = IL_CLAMP(((ILfloat*)Buffer)[i]);
              BytePtr[i] = (ILubyte)(tempFloat * UCHAR_MAX);
            #else
              BytePtr[i] = (ILubyte)(((ILfloat*)Buffer)[i] * UCHAR_MAX);
            #endif
          }
          break;
        case IL_HALF:
          for (i = 0; i < Size; i++) {
            ilHalfToFloatV(ByteBuf + i * 2, &tempFloat);
            #if CLAMP_HALF
              // *((ILuint*)&tempFloat) = ilHalfToFloat(((ILushort*)Buffer)[i]);
              tempFloat = IL_CLAMP(tempFloat);          
              BytePtr[i] = (ILubyte)(tempFloat * UCHAR_MAX);
            #else
              // *((ILuint*)&tempFloat) = ilHalfToFloat(((ILushort*)Buffer)[i]);
              BytePtr[i] = (ILubyte)(tempFloat * UCHAR_MAX);
            #endif
          }
          break;
        case IL_DOUBLE:
          for (i = 0; i < Size; i++) {
            #if CLAMP_DOUBLES
              tempDouble = IL_CLAMP(((ILdouble*)Buffer)[i]);
              BytePtr[i] = (ILubyte)(tempDouble * UCHAR_MAX);
            #else
              BytePtr[i] = (ILubyte)( ((ILdouble*)Buffer)[i] * UCHAR_MAX);
            #endif
          }
          break;
      }
      break;

    case IL_UNSIGNED_SHORT:
    case IL_SHORT:
      ShortPtr = (ILushort*)NewData;
      switch (SrcType)
      {
        case IL_UNSIGNED_BYTE:
        case IL_BYTE:
          for (i = 0; i < Size; i++) {
            ShortPtr[i] = (ILushort)((((ILubyte*)Buffer)[i] << 8) | ((ILubyte*)Buffer)[i]);
          }
          break;
        case IL_UNSIGNED_INT:
        case IL_INT:
          for (i = 0; i < Size; i++) {
            ShortPtr[i] = ((ILuint*)Buffer)[i] >> 16;
          }
          break;
        case IL_FLOAT:
          for (i = 0; i < Size; i++) {
            #if CLAMP_FLOATS
              tempFloat = IL_CLAMP(((ILfloat*)Buffer)[i]);
              ShortPtr[i] = (ILushort)(tempFloat * USHRT_MAX);
            #else
              ShortPtr[i] = (ILushort)( ((ILfloat*)Buffer)[i] * USHRT_MAX);
            #endif
          }
          break;
        case IL_HALF:
          for (i = 0; i < Size; i++) {
            ilHalfToFloatV(ByteBuf + i * 2, &tempFloat);
            #if CLAMP_FLOATS
              // *((ILuint*)&tempFloat) = ilHalfToFloat(((ILushort*)Buffer)[i]);
              tempFloat = IL_CLAMP(tempFloat);          
              ShortPtr[i] = (ILushort)(tempFloat * USHRT_MAX);
            #else
//              *((ILuint*)&tempFloat) = ilHalfToFloat(((ILushort*)Buffer)[i]);
              ShortPtr[i] = (ILushort)(tempFloat * USHRT_MAX);
            #endif
          }
          break;
        case IL_DOUBLE:
          for (i = 0; i < Size; i++) {
            #if CLAMP_DOUBLES
              tempDouble = IL_CLAMP(((ILdouble*)Buffer)[i]);
              ShortPtr[i] = (ILushort)(tempDouble * USHRT_MAX);
            #else
              ShortPtr[i] = (ILushort)( ((ILdouble*)Buffer)[i] * USHRT_MAX);
            #endif
          }
          break;
      }
      break;

    case IL_UNSIGNED_INT:
    case IL_INT:
      IntPtr = (ILuint*)NewData;
      switch (SrcType)
      {
        case IL_UNSIGNED_BYTE:
        case IL_BYTE:
          for (i = 0; i < Size; i++) {
            IntPtr[i] = (ILuint)(
              (((ILubyte*)Buffer)[i] << 24) | (((ILubyte*)Buffer)[i] << 16) | 
              (((ILubyte*)Buffer)[i] << 8)  | ((ILubyte*)Buffer)[i]);
          }
          break;
        case IL_UNSIGNED_SHORT:
        case IL_SHORT:
          for (i = 0; i < Size; i++) {
            IntPtr[i] = (ILuint)((((ILushort*)Buffer)[i] << 16) | ((ILushort*)Buffer)[i]);
          }
          break;
        case IL_FLOAT:
          for (i = 0; i < Size; i++) {
            #if CLAMP_FLOATS
              tempFloat = IL_CLAMP(((ILfloat*)Buffer)[i]);
              IntPtr[i] = (ILuint)(tempFloat * UINT_MAX);
            #else
              IntPtr[i] = (ILuint)( ((ILfloat*)Buffer)[i] * UINT_MAX);
            #endif
          }
          break;
        case IL_HALF:
          for (i = 0; i < Size; i++) {
            ilHalfToFloatV(ByteBuf + i * 2, &tempFloat);
            #if CLAMP_FLOATS
//              *((ILuint*)&tempFloat) = ilHalfToFloat(((ILushort*)Buffer)[i]);
              tempFloat = IL_CLAMP(tempFloat);
              IntPtr[i] = (ILuint)(tempFloat * UINT_MAX);
            #else
//            *((ILuint*)&tempFloat) = ilHalfToFloat(((ILushort*)Buffer)[i]);
            IntPtr[i] = (ILuint)(tempFloat * UINT_MAX);
            #endif
          }
          break;
        case IL_DOUBLE:
          for (i = 0; i < Size; i++) {
            #if CLAMP_DOUBLES
              tempDouble = IL_CLAMP(((ILdouble*)Buffer)[i]);
              IntPtr[i] = (ILuint)(tempDouble * UINT_MAX);
            #else
              IntPtr[i] = (ILuint)( ((ILdouble*)Buffer)[i] * UINT_MAX);
            #endif
          }
          break;
      }
      break;

    // @TODO:  Handle signed better.
    case IL_FLOAT:
      FloatPtr = (ILfloat*)NewData;
      switch (SrcType)
      {
        case IL_UNSIGNED_BYTE:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = ((ILubyte*)Buffer)[i] / (ILfloat)UCHAR_MAX;
          }
          break;
        case IL_BYTE:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = ((ILbyte*)Buffer)[i] / (ILfloat)UCHAR_MAX;
          }
          break;
        case IL_UNSIGNED_SHORT:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = ((ILushort*)Buffer)[i] / (ILfloat)USHRT_MAX;
          }
          break;
        case IL_SHORT:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = ((ILshort*)Buffer)[i] / (ILfloat)USHRT_MAX;
          }
          break;
        case IL_UNSIGNED_INT:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = (ILfloat)((ILuint*)Buffer)[i] / (ILfloat)UINT_MAX;
          }
          break;
        case IL_INT:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = (ILfloat)((ILint*)Buffer)[i] / (ILfloat)UINT_MAX;
          }
          break;
        case IL_HALF:
          for (i = 0; i < Size; i++) {
            ilHalfToFloatV(ByteBuf + i * 2, FloatPtr + i);
            // *((ILuint*)&FloatPtr[i]) = ilHalfToFloat(((ILushort*)Buffer)[i]);
          }
          break;
        case IL_DOUBLE:
          for (i = 0; i < Size; i++) {
            FloatPtr[i] = (ILfloat)((ILdouble*)Buffer)[i];
          }
          break;
      }
      break;

    case IL_DOUBLE:
      DblPtr = (ILdouble*)NewData;
      switch (SrcType)
      {
        case IL_UNSIGNED_BYTE:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILubyte*)Buffer)[i] / (ILdouble)UCHAR_MAX;
          }
          break;
        case IL_BYTE:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILbyte*)Buffer)[i] / (ILdouble)UCHAR_MAX;
          }
          break;
        case IL_UNSIGNED_SHORT:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILushort*)Buffer)[i] / (ILdouble)USHRT_MAX;
          }
          break;
        case IL_SHORT:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILshort*)Buffer)[i] / (ILdouble)USHRT_MAX;
          }
          break;
        case IL_UNSIGNED_INT:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILuint*)Buffer)[i] / (ILdouble)UINT_MAX;
          }
          break;
        case IL_INT:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILint*)Buffer)[i] / (ILdouble)UINT_MAX;
          }
          break;
        case IL_HALF:
          for (i = 0; i < Size; i++) {
            ilHalfToFloatV(ByteBuf + i * 2, &tempFloat);
            // *(ILuint*)&tempFloat = ilHalfToFloat(((ILushort*)Buffer)[i]);
            DblPtr[i] = tempFloat;
          }
          break;
        case IL_FLOAT:
          for (i = 0; i < Size; i++) {
            DblPtr[i] = ((ILfloat*)Buffer)[i];
          }
          break;
      }
      break;
  
    case IL_HALF:
      HalfPtr = (ILushort*)NewData;
      switch (SrcType)
      {
      case IL_UNSIGNED_BYTE:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILubyte*)Buffer)[i] / (ILfloat)UCHAR_MAX;
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          //*((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_BYTE:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILbyte*)Buffer)[i] / (ILfloat)UCHAR_MAX;
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_UNSIGNED_SHORT:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILushort*)Buffer)[i] / (ILfloat)USHRT_MAX;
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_SHORT:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILshort*)Buffer)[i] / (ILfloat)USHRT_MAX;
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_UNSIGNED_INT:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILuint*)Buffer)[i] / (ILfloat)UINT_MAX;
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_INT:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILint*)Buffer)[i] / (ILfloat)UINT_MAX;
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_DOUBLE:
        for (i = 0; i < Size; i++) {
          tempFloat = (ILfloat)((ILdouble*)Buffer)[i];
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      case IL_FLOAT:
        for (i = 0; i < Size; i++) {
          tempFloat = ((ILfloat*)Buffer)[i];
          ilFloatToHalfV(&tempFloat, HalfPtr + i);
          // *((ILushort*)&HalfPtr[i]) = ilFloatToHalf(*(ILuint*)&tempFloat);
        }
        break;
      }
      break;
  }


  return NewData;
}




