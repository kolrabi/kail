//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_rotate.c
//
// Description: Rotates an image.
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"


ILboolean iRotate(ILimage *Image, ILfloat Angle) {
	ILimage	*Temp, *Temp1;
	ILenum	PalType = 0;

	ILimage *  BaseImage = Image;
	if (Image == NULL) {
		iSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->Format == IL_COLOUR_INDEX) {
		PalType = Image->Pal.PalType;
		Image = iConvertImage(Image, iGetPalBaseType(Image->Pal.PalType), IL_UNSIGNED_BYTE);
	}

	Temp = iluRotate_(Image, Angle);
	if (Temp != NULL) {
		if (PalType != 0) {
			iCloseImage(Image);
			Temp1 = iConvertImage(Temp, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			iCloseImage(Temp);
			Temp = Temp1;
		}

		iTexImage(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data);
		if (PalType != 0) {
			Image = BaseImage;
			Image->Pal.PalSize = Temp->Pal.PalSize;
			Image->Pal.PalType = Temp->Pal.PalType;
			Image->Pal.Palette = (ILubyte*)ialloc(Temp->Pal.PalSize);
			if (Image->Pal.Palette == NULL) {
				iCloseImage(Temp);
				return IL_FALSE;
			}
			memcpy(Image->Pal.Palette, Temp->Pal.Palette, Temp->Pal.PalSize);
		}

		Image->Origin = Temp->Origin;
		iCloseImage(Temp);
		return IL_TRUE;
	}
	return IL_FALSE;
}

//! Rotates a bitmap any angle.
//  Code help comes from http://www.leunen.com/cbuilder/rotbmp.html.
ILAPI ILimage* ILAPIENTRY iluRotate_(ILimage *Image, ILfloat Angle)
{
	ILimage		*Rotated = NULL;
	ILuint		x, y, c;
	ILdouble	Cos, Sin;
	ILuint		RotOffset, ImgOffset;
	ILint		MinX, MinY, MaxX, MaxY;
	ILdouble	Point1x, Point1y, Point2x, Point2y, Point3x, Point3y;
	ILint		SrcX, SrcY;

	// Multiples of 90 are special.
	Angle = (ILfloat)fmod((ILdouble)Angle, 360.0);
	if (Angle < 0)
		Angle = 360.0f + Angle;

	Cos = (ILdouble)cos((IL_PI * Angle) / 180.0);
	Sin = (ILdouble)sin((IL_PI * Angle) / 180.0);

	Point1x = (-(ILint)Image->Height * Sin);
	Point1y = (Image->Height * Cos);
	Point2x = (Image->Width * Cos - Image->Height * Sin);
	Point2y = (Image->Height * Cos + Image->Width * Sin);
	Point3x = (Image->Width * Cos);
	Point3y = (Image->Width * Sin);

	MinX = (ILint)IL_MIN(0, IL_MIN(Point1x, IL_MIN(Point2x, Point3x)));
	MinY = (ILint)IL_MIN(0, IL_MIN(Point1y, IL_MIN(Point2y, Point3y)));
	MaxX = (ILint)IL_MAX(Point1x, IL_MAX(Point2x, Point3x));
	MaxY = (ILint)IL_MAX(Point1y, IL_MAX(Point2y, Point3y));

	Rotated = (ILimage*)icalloc(1, sizeof(ILimage));
	if (Rotated == NULL)
		return NULL;
	if (iCopyImageAttr(Rotated, Image) == IL_FALSE) {
		iCloseImage(Rotated);
		return NULL;
	}

	if (iResizeImage(Rotated, (ILuint)(abs(MaxX) - MinX), (ILuint)(abs(MaxY) - MinY), 1, Image->Bpp, Image->Bpc) == IL_FALSE) {
		iCloseImage(Rotated);
		return NULL;
	}

	iClearImage(Rotated);

/*	ShortPtr = (ILushort*)Image->Data;
	IntPtr = (ILuint*)Image->Data;
	DblPtr = (ILdouble*)Image->Data;*/

	//if (iluFilter == ILU_NEAREST) {
	switch (Image->Bpc)
	{
		case 1:  // Byte-based (most images)
			if (Angle == 90.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = x * Rotated->Bps + (Image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							Rotated->Data[RotOffset + c] = Image->Data[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							Rotated->Data[RotOffset + c] = Image->Data[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							Rotated->Data[RotOffset + c] = Image->Data[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < Rotated->Width; x++) {
					for (y = 0; y < Rotated->Height; y++) {
						SrcX = (ILint)(((ILint)x + MinX) * Cos + ((ILint)y + MinY) * Sin);
						SrcY = (ILint)(((ILint)y + MinY) * Cos - ((ILint)x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)Image->Width && SrcY >= 0 && SrcY < (ILint)Image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * Image->Bps + (ILuint)SrcX * Image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								Rotated->Data[RotOffset + c] = Image->Data[ImgOffset + c];
							}
						}
					}
				}
			}
			break;

		case 2:  // Short-based
			Image->Bps /= 2;   // Makes it easier to just
			Rotated->Bps /= 2; //   cast to short.

			if (Angle == 90.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = x * Rotated->Bps + (Image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataUShort(Rotated)[RotOffset + c] = iGetImageDataUShort(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataUShort(Rotated)[RotOffset + c] = iGetImageDataUShort(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataUShort(Rotated)[RotOffset + c] = iGetImageDataUShort(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < Rotated->Width; x++) {
					for (y = 0; y < Rotated->Height; y++) {
						SrcX = (ILint)(((ILint)x + MinX) * Cos + ((ILint)y + MinY) * Sin);
						SrcY = (ILint)(((ILint)y + MinY) * Cos - ((ILint)x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)Image->Width && SrcY >= 0 && SrcY < (ILint)Image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * Image->Bps + (ILuint)SrcX * Image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								iGetImageDataUShort(Rotated)[RotOffset + c] = iGetImageDataUShort(Image)[ImgOffset + c];
							}
						}
					}
				}
			}
			Image->Bps *= 2;
			Rotated->Bps *= 2;
			break;

		case 4:  // Floats or 32-bit integers
			Image->Bps /= 4;
			Rotated->Bps /= 4;

			if (Angle == 90.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = x * Rotated->Bps + (Image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataUInt(Rotated)[RotOffset + c] = iGetImageDataUInt(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataUInt(Rotated)[RotOffset + c] = iGetImageDataUInt(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataUInt(Rotated)[RotOffset + c] = iGetImageDataUInt(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < Rotated->Width; x++) {
					for (y = 0; y < Rotated->Height; y++) {
						SrcX = (ILint)(((ILint)x + MinX) * Cos + ((ILint)y + MinY) * Sin);
						SrcY = (ILint)(((ILint)y + MinY) * Cos - ((ILint)x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)Image->Width && SrcY >= 0 && SrcY < (ILint)Image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * Image->Bps + (ILuint)SrcX * Image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								iGetImageDataUInt(Rotated)[RotOffset + c] = iGetImageDataUInt(Image)[ImgOffset + c];
							}
						}
					}
				}
			}
			Image->Bps *= 4;
			Rotated->Bps *= 4;
			break;

		case 8:  // Double or 64-bit integers
			Image->Bps /= 8;
			Rotated->Bps /= 8;

			if (Angle == 90.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = x * Rotated->Bps + (Image->Width - 1 - y) * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataDouble(Rotated)[RotOffset + c] = iGetImageDataDouble(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 180.0) {
				for (x = 0; x < Image->Width; x++) { 
					for (y = 0; y < Image->Height; y++) { 
						RotOffset = (Image->Height - 1 - y) * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataDouble(Rotated)[RotOffset + c] = iGetImageDataDouble(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else if (Angle == 270.0) {
				for (x = 0; x < Image->Width; x++) {
					for (y = 0; y < Image->Height; y++) {
						RotOffset = (Image->Height - 1 - x) * Rotated->Bps + y * Rotated->Bpp;
						ImgOffset = y * Image->Bps + x * Image->Bpp;
						for (c = 0; c < Rotated->Bpp; c++) {
							iGetImageDataDouble(Rotated)[RotOffset + c] = iGetImageDataDouble(Image)[ImgOffset + c];
						}
					} 
				} 
			}
			else {
				for (x = 0; x < Rotated->Width; x++) {
					for (y = 0; y < Rotated->Height; y++) {
						SrcX = (ILint)(((ILint)x + MinX) * Cos + ((ILint)y + MinY) * Sin);
						SrcY = (ILint)(((ILint)y + MinY) * Cos - ((ILint)x + MinX) * Sin);
						if (SrcX >= 0 && SrcX < (ILint)Image->Width && SrcY >= 0 && SrcY < (ILint)Image->Height) {
							RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
							ImgOffset = (ILuint)SrcY * Image->Bps + (ILuint)SrcX * Image->Bpp;
							for (c = 0; c < Rotated->Bpp; c++) {
								iGetImageDataDouble(Rotated)[RotOffset + c] = iGetImageDataDouble(Image)[ImgOffset + c];
							}
						}
					}
				}
			}
			Image->Bps *= 8;
			Rotated->Bps *= 8;
			break;
	}

	return Rotated;
}

/*
ILAPI ILimage* ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle)
{
	(void)Image; (void)x; (void)y; (void)z; (void)Angle;
	return NULL;
}
*/
