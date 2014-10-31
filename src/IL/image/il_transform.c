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
    return NULL;

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

static inline void *iBufferPixel(void *Data, ILuint x, ILuint y, ILuint z, ILubyte Bpc, ILuint Bps, ILuint SizeOfPlane)
{
  ILuint offset = Bpc*x + y*Bps/Bpc + z*SizeOfPlane;
  return (ILubyte*)Data + offset;
}


//@JASON New routine created 28/03/2001
//! Mirrors an image over its y axis
ILboolean ILAPIENTRY iMirrorImage(ILimage *Image) {
  ILuint x,y,z, pixelSize;
  void *NewData, *OldData;
  
  if (Image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  NewData = ialloc(Image->SizeOfData);
  if (NewData == NULL)
    return IL_FALSE;
  OldData = Image->Data;

  pixelSize = Image->Bpp * Image->Bpc;

  for (z = 0; z < Image->Depth; z++) {
    for (y = 0; y < Image->Height; y++) {
      for (x = 0; x < Image->Width; x++) {
        void *From = iBufferPixel(OldData, Image->Width-1-x, y, z, Image->Bpc, Image->Bps, Image->SizeOfPlane);
        void *To   = iBufferPixel(NewData,                x, y, z, Image->Bpc, Image->Bps, Image->SizeOfPlane);
        memcpy(To, From, pixelSize);
      }
    }    
  }

  ifree(Image->Data);
  Image->Data = NewData;

  return IL_TRUE;
}
