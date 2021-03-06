//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_scale2d.c
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------


// NOTE:  Don't look at this file if you wish to preserve your sanity!


#include "ilu_internal.h"
#include "ilu_states.h"


ILimage *iluScale2DNear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale2DLinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale2DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);

ILimage *iluScale2D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILenum Filter)
{
	if (Image == NULL) {
		iSetError(ILU_ILLEGAL_OPERATION);
		return NULL;
	}

	if (Filter == ILU_NEAREST)
		return iluScale2DNear_(Image, Scaled, Width, Height);
	else if (Filter == ILU_LINEAR)
		return iluScale2DLinear_(Image, Scaled, Width, Height);
	// Filter == ILU_BILINEAR
	return iluScale2DBilinear_(Image, Scaled, Width, Height);
}


ILimage *iluScale2DNear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
  ILdouble  ScaleX = (ILfloat)Width / Image->Width;
  ILdouble  ScaleY = (ILfloat)Height / Image->Height;
	ILuint    ImgBps = Image->Bps / Image->Bpc;
	ILuint    SclBps = Scaled->Bps / Scaled->Bpc;
  ILuint    NewY1, NewY2, NewX1, NewX2, x, y, c;

	switch (Image->Bpc)
	{
		case 1:
			for (y = 0; y < Height; y++) {
				NewY1 = y * SclBps;
				NewY2 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						Scaled->Data[NewY1 + NewX1 + c] = Image->Data[NewY2 + NewX2 + c];
					}
				}
			}
			break;

		case 2:
			for (y = 0; y < Height; y++) {
				NewY1 = y * SclBps;
				NewY2 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						iGetImageDataUShort(Scaled)[NewY1 + NewX1 + c] = iGetImageDataUShort(Image)[NewY2 + NewX2 + c];
					}
				}
			}
			break;

		case 4:
			for (y = 0; y < Height; y++) {
				NewY1 = y * SclBps;
				NewY2 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						iGetImageDataUInt(Scaled)[NewY1 + NewX1 + c] = iGetImageDataUInt(Image)[NewY2 + NewX2 + c];
					}
				}
			}
			break;
			// FIXME: ILdouble
	}

	return Scaled;
}


ILimage *iluScale2DLinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
  ILdouble  ScaleX = (ILfloat)Width / Image->Width;
  ILdouble  ScaleY = (ILfloat)Height / Image->Height;
	ILuint    ImgBps = Image->Bps / Image->Bpc;
	ILuint    SclBps = Scaled->Bps / Scaled->Bpc;
  ILuint    NewY1, NewX1, NewX2, x, y, c, Size;
  ILuint    x1, x2;
  ILdouble  t1, t2, t4, f, ft;

	switch (Image->Bpc)
	{
		case 1:
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					ft = t2 * IL_PI;
					f = (1.0 - cos(ft)) * .5;
					NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = Image->Data[NewY1 + NewX1 + c];
						x2 = Image->Data[NewY1 + NewX2 + c];
						Scaled->Data[Size + c] = (ILubyte)((1.0 - f) * x1 + f * x2);
					}
				}
			}
			break;

		case 2:
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					ft = t2 * IL_PI;
					f = (1.0 - cos(ft)) * .5;
					NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = iGetImageDataUShort(Image)[NewY1 + NewX1 + c];
						x2 = iGetImageDataUShort(Image)[NewY1 + NewX2 + c];
						iGetImageDataUShort(Scaled)[Size + c] = (ILushort)((1.0 - f) * x1 + f * x2);
					}
				}
			}
			break;

		case 4:
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					ft = t2 * IL_PI;
					f = (1.0 - cos(ft)) * .5;
					NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = iGetImageDataUInt(Image)[NewY1 + NewX1 + c];
						x2 = iGetImageDataUInt(Image)[NewY1 + NewX2 + c];
						iGetImageDataUInt(Scaled)[Size + c] = (ILushort)((1.0 - f) * x1 + f * x2);
					}
				}
			}
			break;
	}

	return Scaled;
}


// Rewrote using an algorithm described by Paul Nettle at
//  http://www.gamedev.net/reference/articles/article669.asp.
ILimage *iluScale2DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	ILfloat	  ul, ll, ur, lr;
	ILfloat	  FracX, FracY;
	ILfloat   SrcX, SrcY;
	ILuint	  iSrcX, iSrcY, iSrcXPlus1, iSrcYPlus1, ulOff, llOff, urOff, lrOff;

  ILdouble  ScaleX = (ILfloat)Width / Image->Width;
  ILdouble  ScaleY = (ILfloat)Height / Image->Height;

  ILuint    ImgBps = Image->Bps / Image->Bpc;
  ILuint    SclBps = Scaled->Bps / Scaled->Bpc;

  ILuint    NewY1, NewY2, NewX1, NewX2, Size, x, y, c;
  ILdouble  t1, t2, t3, t4, f, ft, NewX;
  ILdouble  Table[2][4];  // Assumes we don't have larger than 32-bit images.

	// only downscale is allowed
	assert(ScaleX>0 && ScaleX<=1.0f);
	assert(ScaleY>0 && ScaleY<=1.0f);

	// scale factors should match images size
	assert( ((ILfloat)Width -0.5f) / ScaleX < Image->Width );
	assert( ((ILfloat)Height-0.5f) / ScaleY < Image->Height );

	imemclear(Table, sizeof(Table));

	switch (Image->Bpc)
	{
		case 1:
			for (y = 0; y < Height; y++) {
				for (x = 0; x < Width; x++) {
					// Calculate where we want to choose pixels from in our source image.
					SrcX = (ILfloat)(x+0.5f) / (ILfloat)ScaleX;
					SrcY = (ILfloat)(y+0.5f) / (ILfloat)ScaleY;
					// indices of upper-left pixel
					iSrcX = (ILuint)(SrcX-0.5f);
					iSrcY = (ILuint)(SrcY-0.5f);
					// how far SrcX and SrcY are from upper-left pixel center
					FracX = SrcX - (ILfloat)(iSrcX) - 0.5f;
					FracY = SrcY - (ILfloat)(iSrcY) - 0.5f;

					// We do not want to go past the right edge of the image or past the last line in the image,
					//  so this takes care of that.  Normally, iSrcXPlus1 is iSrcX + 1, but if this is past the
					//  right side, we have to bring it back to iSrcX.  The same goes for iSrcYPlus1.
					if (iSrcX < Image->Width - 1)
						iSrcXPlus1 = iSrcX + 1;
					else 
						iSrcXPlus1 = iSrcX;
					if (iSrcY < Image->Height - 1)
						iSrcYPlus1 = iSrcY + 1;
					else
						iSrcYPlus1 = iSrcY;

					// Find out how much we want each of the four pixels contributing to the final values.
					ul = (1.0f - FracX) * (1.0f - FracY);
					ll = (1.0f - FracX) * FracY;
					ur = FracX * (1.0f - FracY);
					lr = FracX * FracY;

					for (c = 0; c < Scaled->Bpp; c++) {
						// We just calculate the offsets for each pixel here...
						ulOff = iSrcY * Image->Bps + iSrcX * Image->Bpp + c;
						llOff = iSrcYPlus1 * Image->Bps + iSrcX * Image->Bpp + c;
						urOff = iSrcY * Image->Bps + iSrcXPlus1 * Image->Bpp + c;
						lrOff = iSrcYPlus1 * Image->Bps + iSrcXPlus1 * Image->Bpp + c;

						// ...and then we do the actual interpolation here.
						Scaled->Data[y * Scaled->Bps + x * Scaled->Bpp + c] = (ILubyte)(
							ul * Image->Data[ulOff] + ll * Image->Data[llOff] + ur * Image->Data[urOff] + lr * Image->Data[lrOff]);
					}
				}
			}
			break;

		case 2:
			Height--;  // Only use regular Height once in the following loop.
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					t3 = (1.0 - t2);
					t4 = t1 * NewX;
					NewX1 = (ILuint)(t4) * Image->Bpp;
					NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						Table[0][c] = t3 * iGetImageDataUShort(Image)[NewY1 + NewX1 + c] +
							t2 * iGetImageDataUShort(Image)[NewY1 + NewX2 + c];

						Table[1][c] = t3 * iGetImageDataUShort(Image)[NewY2 + NewX1 + c] +
							t2 * iGetImageDataUShort(Image)[NewY2 + NewX2 + c];
					}

					// Linearly interpolate between the table values.
					t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
					t3 = (1.0 - t1);
					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						iGetImageDataUShort(Scaled)[Size + c] =
							(ILushort)(t3 * Table[0][c] + t1 * Table[1][c]);
					}
				}
			}

			// Calculate the last row.
			NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
			for (x = 0; x < Width; x++) {
				NewX = Width / ScaleX;
				t1 = x / (ILdouble)Width;
				t4 = t1 * Width;
				ft = (t4 - (ILuint)(t4)) * IL_PI;
				f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
				NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
				NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

				Size = Height * SclBps + x * Image->Bpp;
				for (c = 0; c < Scaled->Bpp; c++) {
					iGetImageDataUShort(Scaled)[Size + c] = (ILushort)((1.0 - f) * iGetImageDataUShort(Image)[NewY1 + NewX1 + c] +
						f * iGetImageDataUShort(Image)[NewY1 + NewX2 + c]);
				}
			}
			break;

		case 4:
			if (Image->Type != IL_FLOAT) {
				Height--;  // Only use regular Height once in the following loop.
				for (y = 0; y < Height; y++) {
					NewY1 = (ILuint)(y / ScaleY) * ImgBps;
					NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;
					for (x = 0; x < Width; x++) {
						NewX = Width / ScaleX;
						t1 = x / (ILdouble)Width;
						t4 = t1 * Width;
						t2 = t4 - (ILuint)(t4);
						t3 = (1.0 - t2);
						t4 = t1 * NewX;
						NewX1 = (ILuint)(t4) * Image->Bpp;
						NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

						for (c = 0; c < Scaled->Bpp; c++) {
							Table[0][c] = t3 * iGetImageDataUInt(Image)[NewY1 + NewX1 + c] +
								t2 * iGetImageDataUInt(Image)[NewY1 + NewX2 + c];

							Table[1][c] = t3 * iGetImageDataUInt(Image)[NewY2 + NewX1 + c] +
								t2 * iGetImageDataUInt(Image)[NewY2 + NewX2 + c];
						}

						// Linearly interpolate between the table values.
						t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
						t3 = (1.0 - t1);
						Size = y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							iGetImageDataUInt(Scaled)[Size + c] =
								(ILuint)(t3 * Table[0][c] + t1 * Table[1][c]);
						}
					}
				}

				// Calculate the last row.
				NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					ft = (t4 - (ILuint)(t4)) * IL_PI;
					f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
					NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
					NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

					Size = Height * SclBps + x * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						iGetImageDataUInt(Scaled)[Size + c] = (ILuint)((1.0 - f) * iGetImageDataUInt(Image)[NewY1 + NewX1 + c] +
							f * iGetImageDataUInt(Image)[NewY1 + NewX2 + c]);
					}
				}
			} else {  // IL_FLOAT
				Height--;  // Only use regular Height once in the following loop.

				for (y = 0; y < Height; y++) {

					NewY1 = (ILuint)(y / ScaleY) * ImgBps;
					NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;

					for (x = 0; x < Width; x++) {
						NewX = Width / ScaleX;

						t1 = x / (ILdouble)Width;
						t4 = t1 * Width;
						t2 = t4 - (ILuint)(t4);
						t3 = (1.0 - t2);
						t4 = t1 * NewX;

						NewX1 = (ILuint)(t4    ) * Image->Bpp;
						NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

						for (c = 0; c < Scaled->Bpp; c++) {
							Table[0][c] = t3 * iGetImageDataFloat(Image)[NewY1 + NewX1 + c] +
								t2 * iGetImageDataFloat(Image)[NewY1 + NewX2 + c];

							Table[1][c] = t3 * iGetImageDataFloat(Image)[NewY2 + NewX1 + c] +
								t2 * iGetImageDataFloat(Image)[NewY2 + NewX2 + c];
						}

						// Linearly interpolate between the table values.
						t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
						t3 = (1.0 - t1);
						Size = y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							iGetImageDataFloat(Scaled)[Size + c] =
								(ILfloat)(t3 * Table[0][c] + t1 * Table[1][c]);
						}
					}
				}

				// Calculate the last row.
				NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					ft = (t4 - (ILuint)(t4)) * IL_PI;
					f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
					NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
					NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

					Size = Height * SclBps + x * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						iGetImageDataFloat(Scaled)[Size + c] = (ILfloat)((1.0 - f) * iGetImageDataFloat(Image)[NewY1 + NewX1 + c] +
							f * iGetImageDataFloat(Image)[NewY1 + NewX2 + c]);
					}
				}
			}

			break;
	}

	return Scaled;
}
