//-----------------------------------------------------------------------------
// Description: Functions for run-length encoding
//-----------------------------------------------------------------------------

// RLE code from TrueVision's TGA sample code available as Tgautils.zip at
//  ftp://ftp.truevision.com/pub/TGA.File.Format.Spec/PC.Version

#include "il_internal.h"
#include "il_rle.h"

static ILuint RleGetPixel(const ILubyte *p, ILuint bpp) {
  ILuint Pixel;
  Pixel = (ILuint)*p++;
  
  while( bpp-- > 1 ) {
    Pixel <<= 8;
    Pixel |= (ILuint)*p++;
  }
  return Pixel;
}

static ILuint RleCountDifferentPixels(const ILubyte *p, ILuint bpp, ILuint pixCnt) {
  ILuint  pixel;
  ILuint  nextPixel = 0;
  ILuint  n = 0;
  ILuint  same = 0;

  if (pixCnt == 0)
    return 0;

  if (pixCnt == 1)
    return 1;

  nextPixel = RleGetPixel(p, bpp);
  while(pixCnt) {
    pixel = nextPixel;
    p += bpp;
    nextPixel = RleGetPixel(p, bpp);
    if (pixel == nextPixel) {
      if (++same > 1)
        break;
    } else {
      same = 0;
    }

    n++; 
    pixCnt --;
  } 

  return n;
}


static ILuint RleCountSamePixels(const ILubyte *p, ILuint bpp, ILuint pixCnt) {
  ILuint  pixel;
  ILuint  nextPixel = 0;
  ILuint  n = 1;

  if (pixCnt == 0)
    return 0;

  if (pixCnt == 1)
    return 1;

  nextPixel = RleGetPixel(p, bpp);
  while(--pixCnt) {
    pixel = nextPixel;
    p += bpp;
    nextPixel = RleGetPixel(p, bpp);
    if (pixel != nextPixel)
      break;

    n++; 
  } 

  return n;

}

ILboolean iRleCompressLine(const ILubyte *p, ILuint n, ILubyte bpp,
      ILubyte *q, ILuint *DestWidth, ILenum CompressMode) {
  
  ILuint  DiffCount;      // pixel count until two identical
  ILuint  SameCount;      // number of identical adjacent pixels
  ILuint  RLEBufSize = 0; // count of number of bytes encoded
  ILuint  MaxRun;

  switch( CompressMode ) {
    case IL_TGACOMP:      MaxRun = TGA_MAX_RUN;     break;
    case IL_SGICOMP:      MaxRun = SGI_MAX_RUN;     break;
    case IL_BMPCOMP:      MaxRun = BMP_MAX_RUN;     break;
    case IL_CUTCOMP:      MaxRun = CUT_MAX_RUN;     break;
    default:  
      iSetError(IL_INVALID_PARAM);
      return IL_FALSE;
  }

  while( n > 0 ) {  
    // Analyze pixels
    DiffCount = RleCountDifferentPixels(p, bpp, n > MaxRun ? MaxRun : n);
    SameCount = RleCountSamePixels     (p, bpp, n > MaxRun ? MaxRun : n);

    if( CompressMode == IL_BMPCOMP ) {
      if (DiffCount == 2) {
        SameCount = 1;
        DiffCount = 0;
      }
    }

    if( DiffCount > 1 ) { // create a raw packet (bmp absolute run)
      switch(CompressMode) {
        case IL_TGACOMP:          *q++ = (ILubyte)(DiffCount-1);
                                  break;

        case IL_BMPCOMP:          *q++ = 0x00; RLEBufSize++;
                                  *q++ = (ILubyte)DiffCount;
                                  break;

        case IL_CUTCOMP:          DiffCount --; *q++ = (ILubyte)DiffCount;
                                  break;
        case IL_SGICOMP:          *q++ = (ILubyte)(DiffCount | 0x80);         
                                  break;
      }
      n -= (ILuint)DiffCount;
      RLEBufSize += (DiffCount * bpp) + 1;

      memcpy(q, p, DiffCount*bpp);
      p += DiffCount*bpp;
      q += DiffCount*bpp;

      if( CompressMode == IL_BMPCOMP ) {
        if( DiffCount % 2 ) {
          *q++ = 0xFF; // insert padding
          RLEBufSize++;
        }
      }
      DiffCount = 0;
      SameCount = 0;
    }

    if( SameCount > 0 ) { // create a RLE packet
      switch(CompressMode) {
        case IL_TGACOMP:          *q++ = (ILubyte)((SameCount - 1) | 0x80);         break;
        case IL_CUTCOMP:          *q++ = (ILubyte)(SameCount | 0x80);               break;
        case IL_SGICOMP:
        case IL_BMPCOMP:          *q++ = (ILubyte)(SameCount);                      break;
      }
      n -= (ILuint)SameCount;
      RLEBufSize += bpp + 1;

      memcpy(q, p, bpp);
      p += SameCount * bpp;
      q += bpp;
    }
  }

  // write line termination code
  switch(CompressMode) {
    case IL_SGICOMP:
      ++RLEBufSize;
      *q++ = 0;
      break;
    case IL_BMPCOMP: 
      *q++ = 0x00; RLEBufSize++;
      *q++ = 0x00; RLEBufSize++;
      break;
  }
  *DestWidth = RLEBufSize;
  
  return IL_TRUE;
}


// Compresses an entire image using run-length encoding
ILuint iRleCompress(const ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp,
    ILubyte *Dest, ILenum CompressMode, ILuint *ScanTable) {
  ILuint i, j, LineLen;
  ILuint DestW        = 0;
  ILuint Bps          = Width * Bpp;
  ILuint SizeOfPlane  = Width * Height * Bpp;

  if( ScanTable )
    imemclear(ScanTable, Depth*Height*sizeof(ILuint));

  for( j = 0; j < Depth; j++ ) {
    for( i = 0; i < Height; i++ ) {
      if( ScanTable )
        *ScanTable++ = DestW;
      iRleCompressLine(Data + j * SizeOfPlane + i * Bps, Width, Bpp, Dest + DestW, &LineLen, CompressMode);
      DestW += LineLen;
    }
  }
  
  if( CompressMode == IL_BMPCOMP ) { // add end of image
    *(Dest+DestW) = 0x00; DestW++;
    *(Dest+DestW) = 0x01; DestW++;
  }

  return DestW;
}
