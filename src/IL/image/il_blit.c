#include "il_internal.h"

ILboolean iBlit(ILimage *Dest, ILimage *Src, ILint DestX,  ILint DestY,   ILint DestZ, 
                                           ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
                                           ILuint Width, ILuint Height, ILuint Depth)
{
  ILuint    x, y, z, ConvBps, ConvSizePlane;
  ILubyte   *Converted;
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
  Converted = (ILubyte*)iConvertBuffer(Src->SizeOfData, Src->Format, Dest->Format, Src->Type, Dest->Type, NULL, SrcTemp);
  if (Converted == NULL)
    return IL_FALSE;
  
  ConvBps       = Dest->Bpp * Src->Width;
  ConvSizePlane = ConvBps   * Src->Height;
  
  // if dest is negative, adjust src and dest accordingly to blit in the
  // overlapped region only
  if (DestX < 0) {
    if (Width <= (ILuint)-DestX) return IL_TRUE; // nothing to do
    Width -= DestX;
    DestX = 0;
  }

  if (DestY < 0) {
    if (Height <= (ILuint)-DestY) return IL_TRUE; // nothing to do
    Height -= DestY;
    DestY = 0;
  }

  if (DestZ < 0) {
    if (Depth <= (ILuint)-DestZ) return IL_TRUE; // nothing to do
    Depth -= DestZ;
    DestZ = 0;
  }
  
  // Limit the copy of data inside of the destination image
  if (Width  + DestX > Dest->Width)  Width  = Dest->Width  - DestX;
  if (Height + DestY > Dest->Height) Height = Dest->Height - DestY;
  if (Depth  + DestZ > Dest->Depth)  Depth  = Dest->Depth  - DestZ;
  
  //@TODO: non funziona con rgba
  if (Src->Format == IL_RGBA || Src->Format == IL_BGRA || Src->Format == IL_LUMINANCE_ALPHA) {
    const ILuint bpp_without_alpha = Dest->Bpp - 1;
    for (z = 0; z < Depth; z++) {
      for (y = 0; y < Height; y++) {
        for (x = 0; x < Width; x++) {
          const ILuint  SrcIndex  = (z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*Dest->Bpp;
          const ILuint  DestIndex = (z+DestZ)*Dest->SizeOfPlane + (y+DestY)*Dest->Bps + (x+DestX)*Dest->Bpp;
          const ILuint  AlphaIdx = SrcIndex + bpp_without_alpha;
          ILfloat FrontAlpha = 0; // foreground opacity
          ILfloat BackAlpha = 0;  // background opacity
          
          switch (Dest->Type)
          {
            case IL_BYTE:
            case IL_UNSIGNED_BYTE:
              FrontAlpha = Converted[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
              BackAlpha = Dest->Data[AlphaIdx]/((float)IL_MAX_UNSIGNED_BYTE);
              break;
            case IL_SHORT:
            case IL_UNSIGNED_SHORT:
              FrontAlpha = ((ILshort*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
              BackAlpha = ((ILshort*)Dest->Data)[AlphaIdx]/((float)IL_MAX_UNSIGNED_SHORT);
              break;
            case IL_INT:
            case IL_UNSIGNED_INT:
              FrontAlpha = ((ILint*)Converted)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
              BackAlpha = ((ILint*)Dest->Data)[AlphaIdx]/((float)IL_MAX_UNSIGNED_INT);
              break;
            case IL_FLOAT:
              FrontAlpha = ((ILfloat*)Converted)[AlphaIdx];
              BackAlpha = ((ILfloat*)Dest->Data)[AlphaIdx];
              break;
            case IL_DOUBLE:
              FrontAlpha = (ILfloat)(((ILdouble*)Converted)[AlphaIdx]);
              BackAlpha = (ILfloat)(((ILdouble*)Dest->Data)[AlphaIdx]);
              break;
          }
          
          // In case of Alpha channel, the data is blended.
          // Computes composite Alpha
          if (DoAlphaBlend)
          {
            ResultAlpha = FrontAlpha + (1.0f - FrontAlpha) * BackAlpha;
            for (c = 0; c < bpp_without_alpha; c++)
            {
              Dest->Data[DestIndex + c] = (ILubyte)( 0.5f + 
                (Converted[SrcIndex + c] * FrontAlpha + 
                (1.0f - FrontAlpha) * Dest->Data[DestIndex + c] * BackAlpha) 
                / ResultAlpha);
            }
            Dest->Data[AlphaIdx] = (ILubyte)(0.5f + ResultAlpha * (float)IL_MAX_UNSIGNED_BYTE);
          }
          else {
            for (c = 0; c < Dest->Bpp; c++)
            {
              Dest->Data[DestIndex + c] = (ILubyte)(Converted[SrcIndex + c]);
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
            Dest->Data[(z+DestZ)*Dest->SizeOfPlane + (y+DestY)*Dest->Bps + (x+DestX)*Dest->Bpp + c] =
             Converted[(z+SrcZ)*ConvSizePlane + (y+SrcY)*ConvBps + (x+SrcX)*Dest->Bpp + c];
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

