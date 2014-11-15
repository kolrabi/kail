
#include "ilu_internal.h"
#include "ilu_states.h"
#include <float.h>
#include <limits.h>


static ILboolean iCrop2D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint Width, ILuint Height) {
  ILuint  x, y, c, OldBps;
  ILubyte *Data;
  ILenum  Origin;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  // Uh-oh, what about 0 dimensions?!
  if (Width > Image->Width || Height > Image->Height) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Data = (ILubyte*)ialloc(Image->SizeOfData);
  if (Data == NULL) {
    return IL_FALSE;
  }

  OldBps = Image->Bps;
  Origin = Image->Origin;
  iCopyPixels(Image, 0, 0, 0, Image->Width, Image->Height, 1, Image->Format, Image->Type, Data);
  if (!iTexImage(Image, Width, Height, Image->Depth, Image->Bpp, Image->Format, Image->Type, NULL)) {
    free(Data);
    return IL_FALSE;
  }
  Image->Origin = Origin;

  // @TODO:  Optimize!  (Especially XOff * Image->Bpp...get rid of it!)
  for (y = 0; y < Image->Height; y++) {
    for (x = 0; x < Image->Bps; x += Image->Bpp) {
      for (c = 0; c < Image->Bpp; c++) {
        Image->Data[y * Image->Bps + x + c] = 
          Data[(y + YOff) * OldBps + x + XOff * Image->Bpp + c];
      }
    }
  }

  ifree(Data);

  return IL_TRUE;
}


static ILboolean iCrop3D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
{
  ILuint  x, y, z, c, OldBps, OldPlane;
  ILubyte *Data;
  ILenum  Origin;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  // Uh-oh, what about 0 dimensions?!
  if (Width > Image->Width || Height > Image->Height || Depth > Image->Depth) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Data = (ILubyte*)ialloc(Image->SizeOfData);
  if (Data == NULL) {
    return IL_FALSE;
  }

  OldBps = Image->Bps;
  OldPlane = Image->SizeOfPlane;
  Origin = Image->Origin;
  iCopyPixels(Image, 0, 0, 0, Image->Width, Image->Height, Image->Depth, Image->Format, Image->Type, Data);
  if (!iTexImage(Image, Width - XOff, Height - YOff, Depth - ZOff, Image->Bpp, Image->Format, Image->Type, NULL)) {
    ifree(Data);
  }
  Image->Origin = Origin;

  for (z = 0; z < Image->Depth; z++) {
    for (y = 0; y < Image->Height; y++) {
      for (x = 0; x < Image->Bps; x += Image->Bpp) {
        for (c = 0; c < Image->Bpp; c++) {
          Image->Data[z * Image->SizeOfPlane + y * Image->Bps + x + c] = 
            Data[(z + ZOff) * OldPlane + (y + YOff) * OldBps + (x + XOff) + c];
        }
      }
    }
  }

  ifree(Data);

  return IL_TRUE;
}


ILboolean iCrop(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth) {
  if (ZOff <= 1)
    return iCrop2D(Image, XOff, YOff, Width, Height);
  return iCrop3D(Image, XOff, YOff, ZOff, Width, Height, Depth);
}


ILboolean iEnlargeCanvas(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILenum Placement) {
  ILubyte *Data/*, Clear[4]*/;
  ILuint  x, y, z, OldBps, OldH, OldD, OldPlane, AddX, AddY;
  ILenum  Origin;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  // Uh-oh, what about 0 dimensions?!
  if (Width < Image->Width || Height < Image->Height || Depth < Image->Depth) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Origin == IL_ORIGIN_LOWER_LEFT) {
    switch (Placement)
    {
      case ILU_LOWER_LEFT:
        AddX = 0;
        AddY = 0;
        break;
      case ILU_LOWER_RIGHT:
        AddX = Width - Image->Width;
        AddY = 0;
        break;
      case ILU_UPPER_LEFT:
        AddX = 0;
        AddY = Height - Image->Height;
        break;
      case ILU_UPPER_RIGHT:
        AddX = Width - Image->Width;
        AddY = Height - Image->Height;
        break;
      case ILU_CENTER:
        AddX = (Width - Image->Width) >> 1;
        AddY = (Height - Image->Height) >> 1;
        break;
      default:
        iSetError(ILU_INVALID_PARAM);
        return IL_FALSE;
    }
  }
  else {  // IL_ORIGIN_UPPER_LEFT
    switch (Placement)
    {
      case ILU_LOWER_LEFT:
        AddX = 0;
        AddY = Height - Image->Height;
        break;
      case ILU_LOWER_RIGHT:
        AddX = Width - Image->Width;
        AddY = Height - Image->Height;
        break;
      case ILU_UPPER_LEFT:
        AddX = 0;
        AddY = 0;
        break;
      case ILU_UPPER_RIGHT:
        AddX = Width - Image->Width;
        AddY = 0;
        break;
      case ILU_CENTER:
        AddX = (Width - Image->Width) >> 1;
        AddY = (Height - Image->Height) >> 1;
        break;
      default:
        iSetError(ILU_INVALID_PARAM);
        return IL_FALSE;
    }
  }

  AddX *= Image->Bpp;

  Data = (ILubyte*)ialloc(Image->SizeOfData);
  if (Data == NULL) {
    return IL_FALSE;
  }

  // Preserve old data.
  OldPlane = Image->SizeOfPlane;
  OldBps   = Image->Bps;
  OldH     = Image->Height;
  OldD     = Image->Depth;
  Origin   = Image->Origin;
  iCopyPixels(Image, 0, 0, 0, Image->Width, Image->Height, OldD, Image->Format, Image->Type, Data);

  iTexImage(Image, Width, Height, Depth, Image->Bpp, Image->Format, Image->Type, NULL);
  Image->Origin = Origin;

  iClearImage(Image);

  for (z = 0; z < OldD; z++) {
    for (y = 0; y < OldH; y++) {
      for (x = 0; x < OldBps; x++) {
        Image->Data[z * Image->SizeOfPlane + (y + AddY) * Image->Bps + x + AddX] =
          Data[z * OldPlane + y * OldBps + x];
      }
    }
  }

  ifree(Data);

  return IL_TRUE;
}

ILboolean iInvertAlpha(ILimage *Image) {
  ILuint    i, NumPix;
  ILubyte   Bpp;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Format != IL_RGBA &&
      Image->Format != IL_BGRA &&
      Image->Format != IL_LUMINANCE_ALPHA) {
      iSetError(ILU_ILLEGAL_OPERATION);
      return IL_FALSE;
  }

  Bpp    = Image->Bpp;
  NumPix = Image->Width * Image->Height * Image->Depth;

  switch (Image->Type) {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      for( i = Bpp - 1; i < NumPix * Bpp; i+=Bpp )
        iGetImageDataUByte(Image)[i] = IL_MAX_UNSIGNED_BYTE - iGetImageDataUByte(Image)[i];
      break;

    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      for( i = Bpp - 1; i < NumPix * Bpp; i+=Bpp )
        iGetImageDataUShort(Image)[i] = IL_MAX_UNSIGNED_SHORT - iGetImageDataUShort(Image)[i];
      break;

    case IL_INT:
    case IL_UNSIGNED_INT:
      for( i = Bpp - 1; i < NumPix * Bpp; i+=Bpp )
        iGetImageDataUInt(Image)[i] = IL_MAX_UNSIGNED_INT - iGetImageDataUInt(Image)[i];
      break;

    case IL_FLOAT:
      for( i = Bpp - 1; i < NumPix * Bpp; i+=Bpp )
        iGetImageDataFloat(Image)[i] = 1.0f - iGetImageDataFloat(Image)[i];
      break;

    case IL_DOUBLE:
      for( i = Bpp - 1; i < NumPix * Bpp; i+=Bpp )
        iGetImageDataDouble(Image)[i] = 1.0 - iGetImageDataDouble(Image)[i];
      break;
  }

  return IL_TRUE;
}


ILboolean iNegative(ILimage *Image) {
  ILuint    i, j, NumPix, Bpp;
  // ILubyte   *Data;
  ILubyte   *RegionMask;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    /*
    if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE) {
      iSetError(ILU_ILLEGAL_OPERATION);
      return IL_FALSE;
    }
    Data = Image->Pal.Palette;
    i = Image->Pal.PalSize;
    */
    // FIXME
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  } else {
    // Data = Image->Data;
    i = Image->SizeOfData;
  }

  RegionMask = iScanFill(Image);
  
  // @TODO:  Optimize this some.

  NumPix = i / Image->Bpc;
  Bpp = Image->Bpp;

  if (RegionMask) {
    ILuint c;
    switch (Image->Type)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              iGetImageDataUByte(Image)[j+c] = IL_MAX_UNSIGNED_BYTE - iGetImageDataUByte(Image)[j+c];
          }
        }
        break;

      case IL_SHORT:
      case IL_UNSIGNED_SHORT:
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              iGetImageDataUShort(Image)[j+c] = IL_MAX_UNSIGNED_SHORT - iGetImageDataUShort(Image)[j+c];
          }
        }
        break;

      case IL_INT:
      case IL_UNSIGNED_INT:
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              iGetImageDataUInt(Image)[j+c] = IL_MAX_UNSIGNED_INT - iGetImageDataUInt(Image)[j+c];
          }
        }
        break;

      case IL_FLOAT:
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              iGetImageDataFloat(Image)[j+c] = 1.0f - iGetImageDataFloat(Image)[j+c];
          }
        }
        break;

      case IL_DOUBLE:
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              iGetImageDataDouble(Image)[j+c] = 1.0f - iGetImageDataDouble(Image)[j+c];
          }
        }
        break;
    }
  }
  else {
    switch (Image->Type)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
        for (j = 0, i = 0; j < NumPix; j ++)
          iGetImageDataUByte(Image)[j] = IL_MAX_UNSIGNED_BYTE - iGetImageDataUByte(Image)[j];
        break;

      case IL_SHORT:
      case IL_UNSIGNED_SHORT:
        for (j = 0, i = 0; j < NumPix; j ++)
          iGetImageDataUShort(Image)[j] = IL_MAX_UNSIGNED_SHORT - iGetImageDataUShort(Image)[j];
        break;

      case IL_INT:
      case IL_UNSIGNED_INT:
        for (j = 0, i = 0; j < NumPix; j ++)
          iGetImageDataUInt(Image)[j] = IL_MAX_UNSIGNED_INT - iGetImageDataUInt(Image)[j];
        break;

      case IL_FLOAT:
        for (j = 0, i = 0; j < NumPix; j ++)
          iGetImageDataFloat(Image)[j] = 1.0f - iGetImageDataFloat(Image)[j];
        break;

      case IL_DOUBLE:
        for (j = 0, i = 0; j < NumPix; j ++)
          iGetImageDataDouble(Image)[j] = 1.0f - iGetImageDataDouble(Image)[j];
        break;
    }
  }

  ifree(RegionMask);

  return IL_TRUE;
}


// Taken from
//  http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue2.html#Insight
//  Hope they don't mind too much. =]
ILboolean iWave(ILimage *Image, ILfloat Angle) {
  ILuint  y;
  ILubyte *TempBuff;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  TempBuff = (ILubyte*)ialloc(Image->SizeOfData);
  if (TempBuff == NULL) {
    return IL_FALSE;
  }

  for (y = 0; y < Image->Height; y++) {
    ILubyte *DataPtr;
    ILint Delta = (ILint)
      (30 * sin((10 * Angle +     y) * IL_DEGCONV) +
       15 * sin(( 7 * Angle + 3 * y) * IL_DEGCONV));

    DataPtr = Image->Data + y * Image->Bps;

    if (Delta < 0) {
      Delta = -Delta;
      memcpy(TempBuff, DataPtr, Image->Bpp * (ILuint)Delta);
      memcpy(DataPtr, DataPtr + Image->Bpp * (ILuint)Delta, Image->Bpp * (Image->Width - (ILuint)Delta));
      memcpy(DataPtr + Image->Bpp * (Image->Width - (ILuint)Delta), TempBuff, Image->Bpp * (ILuint)Delta);
    }
    else if (Delta > 0) {
      memcpy(TempBuff, DataPtr, Image->Bpp * (Image->Width - (ILuint)Delta));
      memcpy(DataPtr, DataPtr + Image->Bpp * (Image->Width - (ILuint)Delta), Image->Bpp * (ILuint)Delta);
      memcpy(DataPtr + Image->Bpp * (ILuint)Delta, TempBuff, Image->Bpp * (Image->Width - (ILuint)Delta));
    }
  }

  ifree(TempBuff);

  return IL_TRUE;
}

/*
ILboolean ILAPIENTRY iSwapColours(ILimage *img) { 
  if( img == NULL ) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (img->Bpp == 1) {
    if (iGetBppPal(img->Pal.PalType) == 0 || img->Format != IL_COLOUR_INDEX) {
      iSetError(ILU_ILLEGAL_OPERATION);  // Can be luminance.
      return IL_FALSE;
    }
    
    switch( img->Pal.PalType ) {
      case IL_PAL_RGB24:
        return iConvertImagePal(img, IL_PAL_BGR24);
      case IL_PAL_RGB32:
        return iConvertImagePal(img, IL_PAL_BGR32);
      case IL_PAL_RGBA32:
        return iConvertImagePal(img, IL_PAL_BGRA32);
      case IL_PAL_BGR24:
        return iConvertImagePal(img, IL_PAL_RGB24);
      case IL_PAL_BGR32:
        return iConvertImagePal(img, IL_PAL_RGB32);
      case IL_PAL_BGRA32:
        return iConvertImagePal(img, IL_PAL_RGBA32);
      default:
        iSetError(ILU_INTERNAL_ERROR);
        return IL_FALSE;
    }
  }

  switch(img->Format) {
    case IL_RGB:
      return iConvertImages(img, IL_BGR, img->Type);
    case IL_RGBA:
      return iConvertImages(img, IL_BGRA, img->Type);
    case IL_BGR:
      return iConvertImages(img, IL_RGB, img->Type);
    case IL_BGRA:
      return iConvertImages(img, IL_RGBA, img->Type);
  }

  iSetError(ILU_INTERNAL_ERROR);
  return IL_FALSE;
}
*/

typedef struct BUCKET { ILuint Colours;  struct BUCKET *Next; } BUCKET;

ILuint iColoursUsed(ILimage *Image) {
  ILuint i, c, Bpp, ColVal, SizeData, BucketPos = 0, NumCols = 0;
  BUCKET Buckets[8192], *Temp;
  union {
    ILubyte bytes[4];
    ILuint  u32;
  } ColTemp;
  ILboolean Matched;
  BUCKET *Heap[9];
  ILuint HeapPos = 0, HeapPtr = 0, HeapSize;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return 0;
  }

  imemclear(Buckets, sizeof(BUCKET) * 8192);
  for (c = 0; c < 9; c++) {
    Heap[c] = 0;
  }

  Bpp = Image->Bpp;
  SizeData = Image->SizeOfData;

  // Create our miniature memory heap.
  // I have determined that the average number of colours versus
  //  the number of pixels is about a 1:8 ratio, so divide by 8.
  HeapSize = IL_MAX(1, Image->SizeOfData / Image->Bpp / 8);
  Heap[0] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
  if (Heap[0] == NULL)
    return IL_FALSE;

  for (i = 0; i < SizeData; i += Bpp) {
    imemclear(&ColTemp, sizeof(ColTemp));

    ColTemp.bytes[0] = Image->Data[i];
    if (Bpp > 1) {
      ColTemp.bytes[1] = Image->Data[i + 1];
      ColTemp.bytes[2] = Image->Data[i + 2];
    }
    if (Bpp > 3)
      ColTemp.bytes[3] = Image->Data[i + 3];

    BucketPos = ColTemp.u32 % 8192;

    // Add to hash table
    if (Buckets[BucketPos].Next == NULL) {
      NumCols++;
      //Buckets[BucketPos].Next = (BUCKET*)ialloc(sizeof(BUCKET));
      Buckets[BucketPos].Next = Heap[HeapPos] + HeapPtr++;
      if (HeapPtr >= HeapSize) {
        Heap[++HeapPos] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
        if (Heap[HeapPos] == NULL)
          goto alloc_error;
        HeapPtr = 0;
      }
      Buckets[BucketPos].Next->Colours = ColTemp.u32;
      Buckets[BucketPos].Next->Next = NULL;
    }
    else {
      Matched = IL_FALSE;
      Temp = Buckets[BucketPos].Next;

      ColVal = ColTemp.u32;
      while (Temp->Next != NULL) {
        if (ColVal == Temp->Colours) {
          Matched = IL_TRUE;
          break;
        }
        Temp = Temp->Next;
      }
      if (!Matched) {
        if (ColVal != Temp->Colours) {  // Check against last entry
          NumCols++;
          Temp = Buckets[BucketPos].Next;
          //Buckets[BucketPos].Next = (BUCKET*)ialloc(sizeof(BUCKET));
          Buckets[BucketPos].Next = Heap[HeapPos] + HeapPtr++;
          if (HeapPtr >= HeapSize) {
            Heap[++HeapPos] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
            if (Heap[HeapPos] == NULL)
              goto alloc_error;
            HeapPtr = 0;
          }
          Buckets[BucketPos].Next->Next = Temp;
          Buckets[BucketPos].Next->Colours = ColTemp.u32;
        }
      }
    }
  }

  // Delete our mini heap.
  for (i = 0; i < 9; i++) {
    if (Heap[i] == NULL)
      break;
    ifree(Heap[i]);
  }

  return NumCols;

alloc_error:
  for (i = 0; i < 9; i++) {
    ifree(Heap[i]);
  }

  return 0;
}

ILboolean iCompareImage(ILimage * Image, ILimage * Original) {
  // Same image, so return true.
  if (Original == Image)
    return IL_TRUE;

  if (Image == NULL || Original == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  // @TODO:  Should we check palettes, too?
  if (Original->Bpp     != Image->Bpp     ||
      Original->Depth   != Image->Depth   ||
      Original->Format  != Image->Format  ||
      Original->Height  != Image->Height  ||
      Original->Origin  != Image->Origin  ||
      Original->Type    != Image->Type    ||
      Original->Width   != Image->Width) {
      return IL_FALSE;
  }

  // different amount of data
  if (Image->SizeOfData != Original->SizeOfData) {
    return IL_FALSE;
  }

  return !memcmp(Image->Data, Original->Data, Image->SizeOfData);
}

ILfloat iSimilarity(ILimage * Image, ILimage * Original) {
  ILfloat Result = 0.0, DiffSum = 0.0f;
  ILimage *Image1, *Image2;
  ILuint  Size, i;
  ILubyte Bpp;
  ILenum Format;

  // Same image, so return true.
  if (Original == Image)
    return 1.0f;

  if (Image == NULL || Original == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return 0.0f;
  }

  // must be same dimensions and layout
  if (Original->Depth   != Image->Depth   ||
      Original->Height  != Image->Height  ||
      Original->Origin  != Image->Origin  ||
      Original->Width   != Image->Width ) {
      iSetError(ILU_ILLEGAL_OPERATION);
      return 0.0f;
  }

  Format = (iFormatHasAlpha(Image->Format) || iFormatHasAlpha(Original->Format)) ?  IL_RGBA : IL_RGB;
  Bpp = iGetBppFormat(Format);

  Image1 = iConvertImage(Image, Format, IL_FLOAT);
  if (!Image1) return 0.0;

  Image2 = iConvertImage(Original, Format, IL_FLOAT);
  if (!Image2) {
    iCloseImage(Image1);
    return 0.0;
  }

  Size = Original->Width * Original->Height * Original->Depth * Bpp;
  for (i = 0; i<Size; i++) {
    ILfloat diff = iGetImageDataFloat(Image1)[i] - iGetImageDataFloat(Image2)[i];
    DiffSum += diff * diff;
  }

  Result = 1.0f - DiffSum / (ILfloat)Size;

  iCloseImage(Image1);
  iCloseImage(Image2);

  return Result;
}


ILboolean iReplaceColour(ILimage *Image, ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance, const ILubyte *ClearCol) {
  ILint TolVal;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return 0;
  }

  if (Tolerance > 1.0f || Tolerance < -1.0f)
    Tolerance = 1.0f;  // Clamp it.
  TolVal = (ILint)(fabs(Tolerance) * UCHAR_MAX);  // To be changed.
  // NumPix = Image->Width * Image->Height * Image->Depth;

  if (Tolerance <= FLT_EPSILON && Tolerance >= 0) {
    //@TODO what is this?
  }
  else {
    ILint Distance, Dist1, Dist2, Dist3;
    ILuint  i;
    switch (Image->Format)
    {
      case IL_RGB:
      case IL_RGBA:
        for (i = 0; i < Image->SizeOfData; i += Image->Bpp) {
          Dist1 = (ILint)Image->Data[i  ] - (ILint)ClearCol[0];
          Dist2 = (ILint)Image->Data[i+1] - (ILint)ClearCol[1];
          Dist3 = (ILint)Image->Data[i+2] - (ILint)ClearCol[2];
          Distance = (ILint)(sqrt(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3));
          if (Distance >= -TolVal && Distance <= TolVal) {
            Image->Data[i] = Red;
            Image->Data[i+1] = Green;
            Image->Data[i+2] = Blue;
          }
        }
        break;
      case IL_BGR:
      case IL_BGRA:
        for (i = 0; i < Image->SizeOfData; i += Image->Bpp) {
          Dist1 = (ILint)Image->Data[i] - (ILint)ClearCol[0];
          Dist2 = (ILint)Image->Data[i+1] - (ILint)ClearCol[1];
          Dist3 = (ILint)Image->Data[i+2] - (ILint)ClearCol[2];
          Distance = (ILint)(sqrt(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3));
          if (Distance >= -TolVal && Distance <= TolVal) {
            Image->Data[i+2] = Red;
            Image->Data[i+1] = Green;
            Image->Data[i] = Blue;
          }
        }
        break;
      case IL_LUMINANCE:
      case IL_LUMINANCE_ALPHA:
        for (i = 0; i < Image->SizeOfData; i += Image->Bpp) {
          Dist1 = (ILint)Image->Data[i] - (ILint)ClearCol[0];
          if (Dist1 >= -TolVal && Dist1 <= TolVal) {
            Image->Data[i] = Blue;
          }
        }
        break;
      //case IL_COLOUR_INDEX:  // @TODO
    }
  }

  return IL_TRUE;
}

// Credit goes to Lionel Brits for this (refer to credits.txt)
ILboolean iEqualize(ILimage *Image) {
  ILuint      Histogram[256]; // image Histogram
  ILuint      SumHistm[256]; // normalized Histogram and LUT
  ILuint      i = 0; // index variable
  ILuint      j = 0; // index variable
  ILuint      Sum = 0;
  ILuint      NumPixels, Bpp;
  ILimage *   LumImage;
  ILuint      NewColour[4];
  ILubyte   * BytePtr;
 
  NewColour[0] = NewColour[1] = NewColour[2] = NewColour[3] = 0;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return 0;
  }

  // @TODO:  Change to work with other types!
  if (Image->Bpc > 1) {
    iSetError(ILU_INTERNAL_ERROR);
    return IL_FALSE;
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    NumPixels = Image->Pal.PalSize / iGetBppPal(Image->Pal.PalType);
    Bpp = iGetBppPal(Image->Pal.PalType);
  } else {
    NumPixels = Image->Width * Image->Height * Image->Depth;
    Bpp = Image->Bpp;
  }

  // Clear the tables.
  imemclear(Histogram, 256 * sizeof(ILuint));
  imemclear(SumHistm,  256 * sizeof(ILuint));

  LumImage = iConvertImage(Image, IL_LUMINANCE, IL_UNSIGNED_BYTE); // the type must be left as it is!
  if (LumImage == NULL)
    return IL_FALSE;
  for (i = 0; i < NumPixels; i++) {
    Histogram[LumImage->Data[i]]++;
  }

  // Calculate normalized Sum of Histogram.
  for (i = 0; i < 256; i++) {
    for (j = 0; j < i; j++)
      Sum += Histogram[j];

    SumHistm[i] = (Sum << 8) / NumPixels;
    Sum = 0;
  }

  BytePtr = (Image->Format == IL_COLOUR_INDEX) ? Image->Pal.Palette : Image->Data;
  // ShortPtr = (ILushort*)Image->Data;
  // IntPtr = (ILuint*)Image->Data;

  // Transform image using new SumHistm as a LUT
  for (i = 0; i < NumPixels; i++) {
    ILint       Intensity;
    ILint       IntensityNew;
    ILfloat     Scale;

    Intensity = LumImage->Data[i];

    // Look up the normalized intensity
    IntensityNew = (ILint)SumHistm[Intensity];

    // Find out by how much the intensity has been Scaled
    Scale = (ILfloat)IntensityNew / (ILfloat)Intensity;

    switch (Image->Bpc)
    {
      case 1:
        // Calculate new pixel(s)
        NewColour[0] = (ILuint)(BytePtr[i * Image->Bpp] * Scale);
        if (Bpp >= 3) {
          NewColour[1] = (ILuint)(BytePtr[i * Image->Bpp + 1] * Scale);
          NewColour[2] = (ILuint)(BytePtr[i * Image->Bpp + 2] * Scale);
        }

        // Clamp values
        if (NewColour[0] > UCHAR_MAX)
          NewColour[0] = UCHAR_MAX;
        if (Bpp >= 3) {
          if (NewColour[1] > UCHAR_MAX)
            NewColour[1] = UCHAR_MAX;
          if (NewColour[2] > UCHAR_MAX)
            NewColour[2] = UCHAR_MAX;
        }

        // Store pixel(s)
        BytePtr[i * Image->Bpp] = (ILubyte)NewColour[0];
        if (Bpp >= 3) {
          BytePtr[i * Image->Bpp + 1] = (ILubyte)NewColour[1];
          BytePtr[i * Image->Bpp + 2] = (ILubyte)NewColour[2];
        }
        break;

      /*case 2:
        // Calculate new pixel
        NewColour[0] = (ILuint)(ShortPtr[i * Image->Bpp] * Scale);
        NewColour[1] = (ILuint)(ShortPtr[i * Image->Bpp + 1] * Scale);
        NewColour[2] = (ILuint)(ShortPtr[i * Image->Bpp + 2] * Scale);

        // Clamp values
        if (NewColour[0] > USHRT_MAX)
          NewColour[0] = USHRT_MAX;
        if (NewColour[1] > USHRT_MAX)
          NewColour[1] = USHRT_MAX;
        if (NewColour[2] > USHRT_MAX)
          NewColour[2] = USHRT_MAX;

        // Store pixel
        ShortPtr[i * Image->Bpp]    = (ILushort)NewColour[0];
        ShortPtr[i * Image->Bpp + 1]  = (ILushort)NewColour[1];
        ShortPtr[i * Image->Bpp + 2]  = (ILushort)NewColour[2];
        break;

      case 4:
        // Calculate new pixel
        NewColour[0] = (ILuint)(IntPtr[i * Image->Bpp] * Scale);
        NewColour[1] = (ILuint)(IntPtr[i * Image->Bpp + 1] * Scale);
        NewColour[2] = (ILuint)(IntPtr[i * Image->Bpp + 2] * Scale);

        // Clamp values
        if (NewColour[0] > UINT_MAX)
          NewColour[0] = UINT_MAX;
        if (NewColour[1] > UINT_MAX)
          NewColour[1] = UINT_MAX;
        if (NewColour[2] > UINT_MAX)
          NewColour[2] = UINT_MAX;

        // Store pixel
        IntPtr[i * 4 * Image->Bpp]    = NewColour[0];
        IntPtr[i * 4 * Image->Bpp + 1]  = NewColour[1];
        IntPtr[i * 4 * Image->Bpp + 2]  = NewColour[2];
        break;*/
    }
  }

  iCloseImage(LumImage);

  return IL_TRUE;
}

ILboolean iNormalize(ILimage *Image) {
  ILuint      i = 0; // index variable
  ILuint      NumData; //, Bpp;
  ILint       Min, Max;
  ILubyte   * BytePtr;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return 0;
  }

  // @TODO:  Change to work with other types!
  if (Image->Bpc > 1) {
    iSetError(ILU_INTERNAL_ERROR);
    return IL_FALSE;
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    NumData = Image->Pal.PalSize;
    // Bpp = iGetBppPal(Image->Pal.PalType);
  } else {
    NumData = Image->SizeOfData;
    // Bpp = Image->Bpp;
  }

  BytePtr = (Image->Format == IL_COLOUR_INDEX) ? Image->Pal.Palette : Image->Data;
  // ShortPtr = (ILushort*)Image->Data;
  // IntPtr = (ILuint*)Image->Data;

  // Transform image using new SumHistm as a LUT
  Min = Max = *BytePtr;

  for (i = 0; i < NumData; i++) {
    if (BytePtr[i] > Max) Max = BytePtr[i];
    if (BytePtr[i] < Min) Min = BytePtr[i];
  }

  for (i = 0; i < NumData; i++) {
    BytePtr[i] = (ILubyte)(255 * ( BytePtr[i] - Min ) / (float)( Max - Min ));
  }

  return IL_TRUE;
}
