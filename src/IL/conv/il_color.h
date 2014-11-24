//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2001-2009 by Denton Woods
// Last modified: 2014-05-23 by Björn Paetzel
//
// Filename: src/IL/il_color.h
//
// Description: Color conversion
//
//-----------------------------------------------------------------------------

#ifndef COLOR_H
#define COLOR_H

// TODO: rename to iPhotoYCC2RGB
INLINE void iYCbCr2RGB(ILubyte Y, ILubyte Cb, ILubyte Cr, ILubyte *r, ILubyte *g, ILubyte *b)
{
  static const ILdouble c11 = 1.4020 * 256.0 / 255.0; // 0.0054980*256;
  static const ILdouble c12 = 0.0000 * 256.0 / 255.0; // 0.0000000*256;
  static const ILdouble c13 = 1.3179 * 256.0 / 255.0; // 0.0051681*256;
  static const ILdouble c21 = 1.4020 * 256.0 / 255.0; // 0.0054980*256;
  static const ILdouble c22 =-0.3939 * 256.0 / 255.0; // 0.0015446*256;
  static const ILdouble c23 =-0.6713 * 256.0 / 255.0; // 0.0026325*256;
  static const ILdouble c31 = 1.4020 * 256.0 / 255.0; // 0.0054980*256;
  static const ILdouble c32 = 2.0281 * 256.0 / 255.0; // 0.0079533*256;
  static const ILdouble c33 = 0.0000 * 256.0 / 255.0; // 0.0000000*256;

  ILint r1, g1, b1;

  r1 = (ILint)(c11*Y + c12*(Cb-156) + c13*(Cr-137)); // 156 -> ~0.61176, 137 -> ~0.53725
  g1 = (ILint)(c21*Y + c22*(Cb-156) + c23*(Cr-137));
  b1 = (ILint)(c31*Y + c32*(Cb-156) + c33*(Cr-137));

  if (r1 < 0)
    *r = 0;
  else if (r1 > 255)
    *r = 255;
  else
    *r = (ILubyte)r1;

  if (g1 < 0)
    *g = 0;
  else if (g1 > 255)
    *g = 255;
  else
    *g = (ILubyte)g1;

  if (b1 < 0)
    *b = 0;
  else if (b1 > 255)
    *b = 255;
  else
    *b = (ILubyte)b1;

  return;
}

INLINE void iRGB2PhotoYCC(ILubyte r, ILubyte g, ILubyte b, ILubyte *Y, ILubyte *Cb, ILubyte *Cr) {
  double L  =  0.299*r/255.0 + 0.587*g/255.0 + 0.114*b/255.0;
  double C1 = -0.299*r/255.0 - 0.587*g/255.0 + 0.886*b/255.0;
  double C2 =  0.701*r/255.0 - 0.587*g/255.0 - 0.114*b/255.0;

  double y  = (255.0/1.402)*L;
  double cb = (111.40) * C1 + 156;
  double cr = (135.64) * C2 + 137;

  if (y<0)
    *Y = 0;
  else if (y>255)
    *Y = 255;
  else 
    *Y = (ILubyte)y;

  if (cb<0)
    *Cb = 0;
  else if (cb>255)
    *Cb = 255;
  else 
    *Cb = (ILubyte)cb;

  if (cr<0)
    *Cr = 0;
  else if (cr>255)
    *Cr = 255;
  else 
    *Cr = (ILubyte)cr;
}

#endif
