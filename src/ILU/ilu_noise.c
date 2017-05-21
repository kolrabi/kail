//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_noise.c
//
// Description: Noise generation functions
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include <math.h>
//#include <time.h>
#include <limits.h>


static ILint iGetNoiseValue(ILuint Factor1, ILuint Factor2) {
  return (ILint)((ILuint)(rand()) % Factor2 - Factor1);
}

ILboolean iNoisify(ILimage *Image, ILclampf Tolerance) {
  ILuint    i, j, c, Factor1, Factor2, NumPix;
  ILint   Val;
  ILubyte   *RegionMask;

  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  RegionMask = iScanFill(Image);

  // @TODO:  Change this to work correctly without time()!
  //srand(time(NULL));
  NumPix = Image->SizeOfData / Image->Bpc;

  switch (Image->Bpc)
  {
    case 1:
      Factor1 = (ILubyte)(Tolerance * IL_MAX_UNSIGNED_BYTE * 0.5f);
      Factor2 = Factor1 + Factor1;
      if (Factor1 == 0) return IL_TRUE;

      for (i = 0, j = 0; i < NumPix; i += Image->Bpp, j++) {
        if (RegionMask) {
          if (!RegionMask[j])
            continue;
        }
        Val = iGetNoiseValue(Factor1, Factor2);
        for (c = 0; c < Image->Bpp; c++) {
          ILint NewVal = (ILint)iGetImageDataUByte(Image)[i + c] + Val;
          if (NewVal > IL_MAX_UNSIGNED_BYTE)
            iGetImageDataUByte(Image)[i + c] = IL_MAX_UNSIGNED_BYTE;
          else if (NewVal < 0)
            iGetImageDataUByte(Image)[i + c] = 0;
          else
            iGetImageDataUByte(Image)[i + c] += Val;
        }
      }
      break;
    case 2:
      Factor1 = (ILushort)(Tolerance * IL_MAX_UNSIGNED_SHORT * 0.5f);
      Factor2 = Factor1 + Factor1;
      if (Factor1 == 0) return IL_TRUE;

      for (i = 0, j = 0; i < NumPix; i += Image->Bpp, j++) {
        if (RegionMask) {
          if (!RegionMask[j])
            continue;
        }
        Val = iGetNoiseValue(Factor1, Factor2);
        for (c = 0; c < Image->Bpp; c++) {
          ILint NewVal = (ILint)iGetImageDataUShort(Image)[i + c] + Val;
          if (NewVal > IL_MAX_UNSIGNED_SHORT)
            iGetImageDataUShort(Image)[i + c] = IL_MAX_UNSIGNED_SHORT;
          else if (NewVal < 0)
            iGetImageDataUShort(Image)[i + c] = 0;
          else
            iGetImageDataUShort(Image)[i + c] = (ILushort)NewVal;
        }
      }
      break;
      // FIXME: ILfloat, ILdouble
    case 4:
      Factor1 = (ILuint)(Tolerance * IL_MAX_UNSIGNED_INT * 0.5f);
      Factor2 = Factor1 + Factor1;
      if (Factor1 == 0) return IL_TRUE;

      for (i = 0, j = 0; i < NumPix; i += Image->Bpp, j++) {
        if (RegionMask) {
          if (!RegionMask[j])
            continue;
        }
        Val = iGetNoiseValue(Factor1, Factor2);
        for (c = 0; c < Image->Bpp; c++) {
          ILint64 NewVal = (ILint64)iGetImageDataUInt(Image)[i + c] + Val;
          if (NewVal > IL_MAX_UNSIGNED_INT)
            iGetImageDataUInt(Image)[i + c] = IL_MAX_UNSIGNED_INT;
          else if (NewVal < 0)
            iGetImageDataUInt(Image)[i + c] = 0;
          else
            iGetImageDataUInt(Image)[i + c] = (ILuint)NewVal;
        }
      }
      break;
  }

  ifree(RegionMask);

  return IL_TRUE;
}




// Information on Perlin Noise taken from
//  http://freespace.virgin.net/hugo.elias/models/m_perlin.htm


/*ILdouble Noise(ILint x, ILint y)
{
    ILint n;
  n = x + y * 57;
    n = (n<<13) ^ n;
    return (1.0 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}


ILdouble SmoothNoise(ILint x, ILint y)
{
  ILdouble corners = ( Noise(x-1, y-1)+Noise(x+1, y-1)+Noise(x-1, y+1)+Noise(x+1, y+1) ) / 16;
  ILdouble sides   = ( Noise(x-1, y)  +Noise(x+1, y)  +Noise(x, y-1)  +Noise(x, y+1) ) /  8;
  ILdouble center  =  Noise(x, y) / 4;
    return corners + sides + center;
}


ILdouble Interpolate(ILdouble a, ILdouble b, ILdouble x)
{
  ILdouble ft = x * 3.1415927;
  ILdouble f = (1 - cos(ft)) * .5;

  return  a*(1-f) + b*f;
}


ILdouble InterpolatedNoise(ILdouble x, ILdouble y)
{
  ILint   integer_X, integer_Y;
  ILdouble  fractional_X, fractional_Y, v1, v2, v3, v4, i1, i2;

  integer_X    = (ILint)x;
  fractional_X = x - integer_X;

  integer_Y    = (ILint)y;
  fractional_Y = y - integer_Y;

  v1 = SmoothNoise(integer_X,     integer_Y);
  v2 = SmoothNoise(integer_X + 1, integer_Y);
  v3 = SmoothNoise(integer_X,     integer_Y + 1);
  v4 = SmoothNoise(integer_X + 1, integer_Y + 1);

  i1 = Interpolate(v1, v2, fractional_X);
  i2 = Interpolate(v3, v4, fractional_X);

  return Interpolate(i1, i2, fractional_Y);
}



ILdouble PerlinNoise(ILdouble x, ILdouble y)
{
  ILuint i, n;
  ILdouble total = 0, p, frequency, amplitude;
  //p = persistence;
  //n = Number_Of_Octaves - 1;
  n = 2;
  //p = .5;

  p = (ILdouble)(rand() % 1000) / 1000.0;


  for (i = 0; i < n; i++) {
    frequency = pow(2, i);
    amplitude = pow(p, i);

    total = total + InterpolatedNoise(x * frequency, y * frequency) * amplitude;
  }

  return total;
}



ILboolean ILAPIENTRY iluNoisify()
{
  ILuint x, y, c;
  ILint Val;

  Image = ilGetCurImage();
  if (Image == NULL) {
    iSetError(ILU_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  for (y = 0; y < Image->Height; y++) {
    for (x = 0; x < Image->Width; x++) {
      Val = (ILint)(PerlinNoise(x, y) * 50.0);

      for (c = 0; c < Image->Bpp; c++) {
        if ((ILint)Image->Data[y * Image->Bps + x * Image->Bpp + c] + Val > 255)
          Image->Data[y * Image->Bps + x * Image->Bpp + c] = 255;
        else if ((ILint)Image->Data[y * Image->Bps + x * Image->Bpp + c] + Val < 0)
          Image->Data[y * Image->Bps + x * Image->Bpp + c] = 0;
        else
          Image->Data[y * Image->Bps + x * Image->Bpp + c] += Val;
      }
    }
  }

  return IL_TRUE;
}*/
