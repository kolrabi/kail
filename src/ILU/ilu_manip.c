
#include "ilu_internal.h"
#include "ilu_states.h"
#include <float.h>
#include <limits.h>


ILboolean iCrop2D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint Width, ILuint Height) {
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


ILboolean iCrop3D(ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
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

ILboolean ILAPIENTRY iFlipImage(ILimage *image) {
  if( image == NULL ) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  iFlipBuffer(image->Data,image->Depth,image->Bps,image->Height);

  return IL_TRUE;
}

ILboolean iInvertAlpha(ILimage *Image) {
  ILuint    i, *IntPtr, NumPix;
  ILubyte   *Data;
  ILushort  *ShortPtr;
  ILfloat   *FltPtr;
  ILdouble  *DblPtr;
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

  Data   = Image->Data;
  Bpp    = Image->Bpp;
  NumPix = Image->Width * Image->Height * Image->Depth;

  switch (Image->Type) {
    case IL_BYTE:
    case IL_UNSIGNED_BYTE:
      Data += (Bpp - 1);
      for( i = Bpp - 1; i < NumPix; i++, Data += Bpp )
        *(Data) = ~*(Data);
      break;

    case IL_SHORT:
    case IL_UNSIGNED_SHORT:
      ShortPtr = ((ILushort*)Data) + Bpp-1; 
      for (i = Bpp - 1; i < NumPix; i++, ShortPtr += Bpp)
        *(ShortPtr) = ~*(ShortPtr);
      break;

    case IL_INT:
    case IL_UNSIGNED_INT:
      IntPtr = ((ILuint*)Data) + Bpp-1;
      for (i = Bpp - 1; i < NumPix; i++, IntPtr += Bpp)
        *(IntPtr) = ~*(IntPtr);
      break;

    case IL_FLOAT:
      FltPtr = ((ILfloat*)Data) + Bpp - 1;
      for (i = Bpp - 1; i < NumPix; i++, FltPtr += Bpp)
        *(FltPtr) = 1.0f - *(FltPtr);
      break;

    case IL_DOUBLE:
      DblPtr = ((ILdouble*)Data) + Bpp - 1;
      for (i = Bpp - 1; i < NumPix; i++, DblPtr += Bpp)
        *(DblPtr) = 1.0f - *(DblPtr);
      break;
  }

  return IL_TRUE;
}


ILboolean iNegative(ILimage *Image) {
  ILuint    i, j, c, *IntPtr, NumPix, Bpp;
  ILubyte   *Data;
  ILushort  *ShortPtr;
  ILubyte   *RegionMask;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (Image->Format == IL_COLOUR_INDEX) {
    if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE) {
      iSetError(ILU_ILLEGAL_OPERATION);
      return IL_FALSE;
    }
    Data = Image->Pal.Palette;
    i = Image->Pal.PalSize;
  }
  else {
    Data = Image->Data;
    i = Image->SizeOfData;
  }

  RegionMask = iScanFill(Image);
  
  // @TODO:  Optimize this some.

  NumPix = i / Image->Bpc;
  Bpp = Image->Bpp;

  if (RegionMask) {
    switch (Image->Bpc)
    {
      case 1:
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++, Data += Bpp) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              *(Data+c) = ~*(Data+c);
          }
        }
        break;

      case 2:
        ShortPtr = (ILushort*)Data;
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++, ShortPtr += Bpp) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              *(ShortPtr+c) = ~*(ShortPtr+c);
          }
        }
        break;

      case 4:
        IntPtr = (ILuint*)Data;
        for (j = 0, i = 0; j < NumPix; j += Bpp, i++, IntPtr += Bpp) {
          for (c = 0; c < Bpp; c++) {
            if (RegionMask[i])
              *(IntPtr+c) = ~*(IntPtr+c);
          }
        }
        break;
    }
  }
  else {
    switch (Image->Bpc)
    {
      case 1:
        for (j = 0; j < NumPix; j++, Data++) {
          *(Data) = ~*(Data);
        }
        break;

      case 2:
        ShortPtr = (ILushort*)Data;
        for (j = 0; j < NumPix; j++, ShortPtr++) {
          *(ShortPtr) = ~*(ShortPtr);
        }
        break;

      case 4:
        IntPtr = (ILuint*)Data;
        for (j = 0; j < NumPix; j++, IntPtr++) {
          *(IntPtr) = ~*(IntPtr);
        }
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
  ILint Delta;
  ILuint  y;
  ILubyte *DataPtr, *TempBuff;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  TempBuff = (ILubyte*)ialloc(Image->SizeOfData);
  if (TempBuff == NULL) {
    return IL_FALSE;
  }

  for (y = 0; y < Image->Height; y++) {
    Delta = (ILint)
      (30 * sin((10 * Angle +     y) * IL_DEGCONV) +
       15 * sin(( 7 * Angle + 3 * y) * IL_DEGCONV));

    DataPtr = Image->Data + y * Image->Bps;

    if (Delta < 0) {
      Delta = -Delta;
      memcpy(TempBuff, DataPtr, Image->Bpp * Delta);
      memcpy(DataPtr, DataPtr + Image->Bpp * Delta, Image->Bpp * (Image->Width - Delta));
      memcpy(DataPtr + Image->Bpp * (Image->Width - Delta), TempBuff, Image->Bpp * Delta);
    }
    else if (Delta > 0) {
      memcpy(TempBuff, DataPtr, Image->Bpp * (Image->Width - Delta));
      memcpy(DataPtr, DataPtr + Image->Bpp * (Image->Width - Delta), Image->Bpp * Delta);
      memcpy(DataPtr + Image->Bpp * Delta, TempBuff, Image->Bpp * (Image->Width - Delta));
    }
  }

  ifree(TempBuff);

  return IL_TRUE;
}

ILboolean ILAPIENTRY iSwapColours(ILimage *img) { 
  if( img == NULL ) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  if (img->Bpp == 1) {
    if (ilGetBppPal(img->Pal.PalType) == 0 || img->Format != IL_COLOUR_INDEX) {
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


typedef struct BUCKET { ILubyte Colours[4];  struct BUCKET *Next; } BUCKET;

ILuint iColoursUsed(ILimage *Image) {
  ILuint i, c, Bpp, ColVal, SizeData, BucketPos = 0, NumCols = 0;
  BUCKET Buckets[8192], *Temp;
  ILubyte ColTemp[4];
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
    *(ILuint*)ColTemp = 0;
    /*for (c = 0; c < Bpp; c++) {
      ColTemp[c] = Image->Data[i + c];
    }*/
    ColTemp[0] = Image->Data[i];
    if (Bpp > 1) {
      ColTemp[1] = Image->Data[i + 1];
      ColTemp[2] = Image->Data[i + 2];
    }
    if (Bpp > 3)
      ColTemp[3] = Image->Data[i + 3];

    BucketPos = *(ILuint*)ColTemp % 8192;

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
      *(ILuint*)Buckets[BucketPos].Next->Colours = *(ILuint*)ColTemp;
      Buckets[BucketPos].Next->Next = NULL;
    }
    else {
      Matched = IL_FALSE;
      Temp = Buckets[BucketPos].Next;

      ColVal = *(ILuint*)ColTemp;
      while (Temp->Next != NULL) {
        if (ColVal == *(ILuint*)Temp->Colours) {
          Matched = IL_TRUE;
          break;
        }
        Temp = Temp->Next;
      }
      if (!Matched) {
        if (ColVal != *(ILuint*)Temp->Colours) {  // Check against last entry
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
          *(ILuint*)Buckets[BucketPos].Next->Colours = *(ILuint*)ColTemp;
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

ILboolean iReplaceColour(ILimage *Image, ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance, const ILubyte *ClearCol) {
  ILint TolVal, Distance, Dist1, Dist2, Dist3;
  ILuint  i; //, NumPix;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return 0;
  }

  if (Tolerance > 1.0f || Tolerance < -1.0f)
    Tolerance = 1.0f;  // Clamp it.
  TolVal = (ILuint)(fabs(Tolerance) * UCHAR_MAX);  // To be changed.
  // NumPix = Image->Width * Image->Height * Image->Depth;

  if (Tolerance <= FLT_EPSILON && Tolerance >= 0) {
    //@TODO what is this?
  }
  else {
    switch (Image->Format)
    {
      case IL_RGB:
      case IL_RGBA:
        for (i = 0; i < Image->SizeOfData; i += Image->Bpp) {
          Dist1 = (ILint)Image->Data[i  ] - (ILint)ClearCol[0];
          Dist2 = (ILint)Image->Data[i+1] - (ILint)ClearCol[1];
          Dist3 = (ILint)Image->Data[i+2] - (ILint)ClearCol[2];
          Distance = (ILint)sqrt((float)(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3));
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
          Distance = (ILint)sqrt((float)(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3));
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
  ILint       Intensity;
  ILfloat     Scale;
  ILint       IntensityNew;
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
    NumPixels = Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType);
    Bpp = ilGetBppPal(Image->Pal.PalType);
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

  ilCloseImage(LumImage);

  return IL_TRUE;
}
