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

INLINE void iYCbCr2RGB(ILubyte Y, ILubyte Cb, ILubyte Cr, ILubyte *r, ILubyte *g, ILubyte *b)
{
  static const ILdouble c11 = 0.0054980*256;
  static const ILdouble c12 = 0.0000000*256;
  static const ILdouble c13 = 0.0051681*256;
  static const ILdouble c21 = 0.0054980*256;
  static const ILdouble c22 =-0.0015446*256;
  static const ILdouble c23 =-0.0026325*256;
  static const ILdouble c31 = 0.0054980*256;
  static const ILdouble c32 = 0.0079533*256;
  static const ILdouble c33 = 0.0000000*256;
  ILint r1, g1, b1;

  r1 = (ILint)(c11*Y + c12*(Cb-156) + c13*(Cr-137));
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

#endif
