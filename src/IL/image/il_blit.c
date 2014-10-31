#include "il_internal.h"

ILboolean iBlit(ILimage *Dest, ILimage *Src, ILint DestX,  ILint DestY,   ILint DestZ, 
                                           ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
                                           ILuint Width, ILuint Height, ILuint Depth)
{
  ILuint    x, y, z, ConvBps, ConvSizePlane;
  void   *Converted, *DestData;
  ILuint    c;
  // ILuint   StartX, StartY, StartZ;
  ILboolean DestFlipped = IL_FALSE;
  ILboolean DoAlphaBlend = IL_FALSE;
  ILubyte   *SrcTemp;
  ILfloat   ResultAlpha;

  // Check if the destination and source really exist
  if (Dest == NULL || Src == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  // set the destination image to upper left origin
  if (Dest->Origin == IL_ORIGIN_LOWER_LEFT) {  // Dest
    DestFlipped = IL_TRUE;
    iFlipImage(Dest);
  }
  //determine alpha support
  DoAlphaBlend = ilIsEnabled(IL_BLIT_BLEND);

  //@TODO test if coordinates are inside the images (hard limit for source)
  
  // set the source image to upper left origin
  if (Src->Origin == IL_ORIGIN_LOWER_LEFT)
  {
    SrcTemp = iGetFlipped(Src);
    if (SrcTemp == NULL)
    {
      if (DestFlipped)
        iFlipImage(Dest);
      return IL_FALSE;
    }
  }
  else
  {
    SrcTemp = Src->Data;
  }
  
  // convert source image to match the destination image type and format
  Converted = iConvertBuffer(Src->SizeOfData, Src->Format, Dest->Format, Src->Type, Dest->Type, NULL, SrcTemp);
  if (Converted == NULL)
    return IL_FALSE;
  DestData = Dest->Data;
  
  ConvBps       = Dest->Bpp * Src->Width;
  ConvSizePlane = ConvBps   * Src->Height;
  
  // if dest is negative, adjust src and dest accordingly to blit in the
  // overlapped region only
  if (DestX < 0) {
    if (Width <= (ILuint)-DestX) return IL_TRUE; // nothing to do
    Width -= (ILuint)-DestX;
    SrcX += (ILuint)-DestX;
    DestX = 0;
  }

  if (DestY < 0) {
    if (Height <= (ILuint)-DestY) return IL_TRUE; // nothing to do
    Height -= (ILuint)-DestY;
    SrcY += (ILuint)-DestY;
    DestY = 0;
  }

  if (DestZ < 0) {
    if (Depth <= (ILuint)-DestZ) return IL_TRUE; // nothing to do
    Depth -= (ILuint)-DestZ;
    SrcZ += (ILuint)-DestZ;
    DestZ = 0;
  }
  
  // Limit the copy of data inside of the destination image
  if (Width  + (ILuint)DestX > Dest->Width)  Width  = Dest->Width  - (ILuint)DestX;
  if (Height + (ILuint)DestY > Dest->Height) Height = Dest->Height - (ILuint)DestY;
  if (Depth  + (ILuint)DestZ > Dest->Depth)  Depth  = Dest->Depth  - (ILuint)DestZ;
  
  //@TODO: non funziona con rgba
  if (Src->Format == IL_RGBA || Src->Format == IL_BGRA || Src->Format == IL_LUMINANCE_ALPHA) {
    const ILuint bpp_without_alpha = Dest->Bpp - 1;
    for (z = 0; z < Depth; z++) {
      for (y = 0; y < Height; y++) {
        for (x = 0; x < Width; x++) {
          const ILuint  SrcIndex  = (z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*Dest->Bpp;
          const ILuint  DestIndex = (z+(ILuint)DestZ)*Dest->SizeOfPlane + (y+(ILuint)DestY)*Dest->Bps + (x+(ILuint)DestX)*Dest->Bpp;
          const ILuint  AlphaIdx = SrcIndex + bpp_without_alpha;
          ILfloat FrontAlpha = 0; // foreground opacity
          ILfloat BackAlpha = 0;  // background opacity
          
          switch (Dest->Type)
          {
            case IL_BYTE:
            case IL_UNSIGNED_BYTE:
              FrontAlpha = ((ILbyte*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
              BackAlpha  = ((ILbyte*)DestData )[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
              break;
            case IL_SHORT:
            case IL_UNSIGNED_SHORT:
              FrontAlpha = ((ILshort*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
              BackAlpha  = ((ILshort*)DestData )[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
              break;
            case IL_INT:
            case IL_UNSIGNED_INT:
              FrontAlpha = ((ILint*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
              BackAlpha  = ((ILint*)DestData )[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
              break;
            case IL_FLOAT:
              FrontAlpha = ((ILfloat*)Converted)[AlphaIdx];
              BackAlpha  = ((ILfloat*)DestData )[AlphaIdx];
              break;
            case IL_DOUBLE:
              FrontAlpha = (ILfloat)(((ILdouble*)Converted)[AlphaIdx]);
              BackAlpha  = (ILfloat)(((ILdouble*)DestData )[AlphaIdx]);
              break;
          }
          
          // In case of Alpha channel, the data is blended.
          // Computes composite Alpha
          if (DoAlphaBlend)
          {
            ResultAlpha = FrontAlpha + (1.0f - FrontAlpha) * BackAlpha;
            for (c = 0; c < bpp_without_alpha; c++)
            {
              ((ILubyte*)DestData)[DestIndex + c] = (ILubyte)( 
                0.5f + (
                  ((ILubyte*)Converted)[SrcIndex + c]  * FrontAlpha + 
                  ((ILubyte*)DestData )[DestIndex + c] * BackAlpha  * (1.0f - FrontAlpha)
                ) / ResultAlpha
              );
            }
            ((ILubyte*)DestData)[AlphaIdx] = (ILubyte)(0.5f + ResultAlpha * (float)IL_MAX_UNSIGNED_BYTE);
          }
          else {
            for (c = 0; c < Dest->Bpp; c++)
            {
              ((ILubyte*)DestData)[DestIndex + c] = (ILubyte)(((ILubyte*)Converted)[SrcIndex + c]);
            }
          }
        }
      }
    }
  } else {
    for( z = 0; z < Depth; z++ ) {
      for( y = 0; y < Height; y++ ) {
        for( x = 0; x < Width; x++ ) {
          for( c = 0; c < Dest->Bpp; c++ ) {
            ((ILubyte*)DestData)  [(z+(ILuint)DestZ)*Dest->SizeOfPlane + 
                                   (y+(ILuint)DestY)*Dest->Bps         + 
                                   (x+(ILuint)DestX)*Dest->Bpp         + c] =
              ((ILubyte*)Converted)[ (z+SrcZ)*ConvSizePlane + 
                                     (y+SrcY)*ConvBps + 
                                     (x+SrcX)*Dest->Bpp + c];
          }
        }
      }
    }
  }
  
  if (SrcTemp != Src->Data)
    ifree(SrcTemp);
  
  if (DestFlipped)
    iFlipImage(Dest);
  
  ifree(Converted);
  return IL_TRUE;
}

ILboolean iOverlayImage(ILimage *Dest, ILimage *Src, ILint XCoord, ILint YCoord, ILint ZCoord) {
  ILuint  Width, Height, Depth;

  // Check if the destination and source really exist
  if (Dest == NULL || Src == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  Width = Dest->Width;  
  Height = Dest->Height;  
  Depth = Dest->Depth;

  return iBlit(Dest, Src, XCoord, YCoord, ZCoord, 0, 0, 0, Width, Height, Depth);
}

