#include "il_internal.h"


ILAPI void ILAPIENTRY iFlipBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num)
{
  ILubyte *StartPtr, *EndPtr;
  ILuint y, d;
  const ILuint size = line_num * line_size;

  for (d = 0; d < depth; d++) {
    StartPtr = buff + d * size;
    EndPtr   = buff + d * size + size;

    for (y = 0; y < (line_num/2); y++) {
      EndPtr -= line_size; 
      iMemSwap(StartPtr, EndPtr, line_size);
      StartPtr += line_size;
    }
  }
}

// Just created for internal use.
static ILubyte* iFlipNewBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num)
{
  ILubyte *data;
  ILubyte *s1, *s2;
  ILuint y, d;
  const ILuint size = line_num * line_size;

  if ((data = (ILubyte*)ialloc(depth*size)) == NULL)
    return IL_FALSE;

  for (d = 0; d < depth; d++) {
    s1 = buff + d * size;
    s2 = data + d * size+size;

    for (y = 0; y < line_num; y++) {
      s2 -= line_size; 
      memcpy(s2,s1,line_size);
      s1 += line_size;
    }
  }
  return data;
}


// Flips an image over its x axis
ILboolean ILAPIENTRY iFlipImage(ILimage *Image)
{
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Image->Origin = (Image->Origin == IL_ORIGIN_LOWER_LEFT) ?
            IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;

  iFlipBuffer(Image->Data, Image->Depth, Image->Bps, Image->Height);

  return IL_TRUE;
}

// Just created for internal use.
ILubyte* ILAPIENTRY iGetFlipped(ILimage *img)
{
  if (img == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }
  return iFlipNewBuffer(img->Data,img->Depth,img->Bps,img->Height);
}


//@JASON New routine created 28/03/2001
//! Mirrors an image over its y axis
ILboolean ILAPIENTRY iMirrorImage(ILimage *Image) {
  ILubyte   *Data, *DataPtr, *Temp;
  ILuint    y, d, PixLine;
  ILint   x, c;
  ILushort  *ShortPtr, *TempShort;
  ILuint    *IntPtr, *TempInt;
  ILdouble  *DblPtr, *TempDbl;

  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Data = (ILubyte*)ialloc(Image->SizeOfData);
  if (Data == NULL)
    return IL_FALSE;

  PixLine = Image->Bps / Image->Bpc;
  switch (Image->Bpc)
  {
    case 1:
      Temp = Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        DataPtr = Data + d * Image->SizeOfPlane;
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, Temp++) {
              DataPtr[y * PixLine + x * Image->Bpp + c] = *Temp;
            }
          }
        }
      }
      break;

    case 2:
      TempShort = (ILushort*)Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        ShortPtr = (ILushort*)(Data + d * Image->SizeOfPlane);
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, TempShort++) {
              ShortPtr[y * PixLine + x * Image->Bpp + c] = *TempShort;
            }
          }
        }
      }
      break;

    case 4:
      TempInt = (ILuint*)Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        IntPtr = (ILuint*)(Data + d * Image->SizeOfPlane);
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, TempInt++) {
              IntPtr[y * PixLine + x * Image->Bpp + c] = *TempInt;
            }
          }
        }
      }
      break;

    case 8:
      TempDbl = (ILdouble*)Image->Data;
      for (d = 0; d < Image->Depth; d++) {
        DblPtr = (ILdouble*)(Data + d * Image->SizeOfPlane);
        for (y = 0; y < Image->Height; y++) {
          for (x = Image->Width - 1; x >= 0; x--) {
            for (c = 0; c < Image->Bpp; c++, TempDbl++) {
              DblPtr[y * PixLine + x * Image->Bpp + c] = *TempDbl;
            }
          }
        }
      }
      break;
  }

  ifree(Image->Data);
  Image->Data = Data;

  return IL_TRUE;
}
