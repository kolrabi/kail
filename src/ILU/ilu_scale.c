//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 12/27/2008
//
// Filename: src-ILU/src/ilu_scale.c
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"


ILboolean ILAPIENTRY iluEnlargeImage(ILfloat XDim, ILfloat YDim, ILfloat ZDim)
{
	if (XDim <= 0.0f || YDim <= 0.0f || ZDim <= 0.0f) {
		iSetError(ILU_INVALID_PARAM);
		return IL_FALSE;
	}

	ILimage *  Image = iGetCurImage();
	return iluScale((ILuint)(Image->Width * XDim), (ILuint)(Image->Height * YDim),
					(ILuint)(Image->Depth * ZDim));
}


ILimage *iluScale1D_(ILimage *Image, ILimage *Scaled, ILuint Width);
ILimage *iluScale2D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale3D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);


ILboolean ILAPIENTRY iluScale(ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage		*Temp;
	ILboolean	UsePal;
	ILenum		PalType;
	ILenum		Origin;

	ILimage *  Image = iGetCurImage();
	if (Image == NULL) {
		iSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->Width == Width && Image->Height == Height && Image->Depth == Depth)
		return IL_TRUE;

	// A parameter of 0 is not valid.  Let's just assume that the user wanted a value of 1 instead.
	if (Width == 0)  Width = 1;
	if (Height == 0) Height = 1;
	if (Depth == 0)  Depth = 1;

	if ((Image->Width<Width) || (Image->Height<Height)) // only do special scale if there is some zoom?
	{
		switch (iluFilter)
		{
			case ILU_SCALE_BOX:
			case ILU_SCALE_TRIANGLE:
			case ILU_SCALE_BELL:
			case ILU_SCALE_BSPLINE:
			case ILU_SCALE_LANCZOS3:
			case ILU_SCALE_MITCHELL:

				Image = iGetCurImage();
				if (Image == NULL) {
					iSetError(ILU_ILLEGAL_OPERATION);
					return IL_FALSE;
				}

				// Not supported yet.
				if (Image->Type != IL_UNSIGNED_BYTE ||
					Image->Format == IL_COLOUR_INDEX ||
					Image->Depth > 1) {
						iSetError(ILU_ILLEGAL_OPERATION);
						return IL_FALSE;
				}

				if (Image->Width > Width) // shrink width first
				{
					Origin = Image->Origin;
					Temp = iluScale_(Image, Width, Image->Height, Image->Depth);
					if (Temp != NULL) {
						if (!ilTexImage_(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
							ilCloseImage(Temp);
							return IL_FALSE;
						}
						Image->Origin = Origin;
						ilCloseImage(Temp);
					}
				}
				else if (Image->Height > Height) // shrink height first
				{
					Origin = Image->Origin;
					Temp = iluScale_(Image, Image->Width, Height, Image->Depth);
					if (Temp != NULL) {
						if (!ilTexImage_(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
							ilCloseImage(Temp);
							return IL_FALSE;
						}
						Image->Origin = Origin;
						ilCloseImage(Temp);
					}
				}

				return (ILboolean)iluScaleAdvanced(Width, Height, iluFilter);
		}
	}


	Origin = Image->Origin;
	UsePal = (Image->Format == IL_COLOUR_INDEX);
	PalType = Image->Pal.PalType;
	Temp = iluScale_(Image, Width, Height, Depth);
	if (Temp != NULL) {
		if (!ilTexImage_(Image, Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data)) {
			ilCloseImage(Temp);
			return IL_FALSE;
		}
		Image->Origin = Origin;
		ilCloseImage(Temp);
		if (UsePal) {
			if (!ilConvertImage(IL_COLOUR_INDEX, IL_UNSIGNED_BYTE))
				return IL_FALSE;
			ilConvertPal(PalType);
		}
		return IL_TRUE;
	}

	return IL_FALSE;
}


ILAPI ILimage* ILAPIENTRY iluScale_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage	*Scaled, *CurImage, *ToScale;
	ILenum	Format; // , PalType;

	CurImage = iGetCurImage();
	Format = Image->Format;
	if (Format == IL_COLOUR_INDEX) {
		ilSetCurImage(Image);
		// PalType = Image->Pal.PalType;
		ToScale = iConvertImage(Image, ilGetPalBaseType(Image->Pal.PalType), Image->Type);
	}
	else {
		ToScale = Image;
	}

	// So we don't replicate this 3 times (one in each iluScalexD_() function.
	Scaled = (ILimage*)icalloc(1, sizeof(ILimage));
	if (ilCopyImageAttr(Scaled, ToScale) == IL_FALSE) {
		ilCloseImage(Scaled);
		if (ToScale != Image)
			ilCloseImage(ToScale);
		ilSetCurImage(CurImage);
		return NULL;
	}
	if (ilResizeImage(Scaled, Width, Height, Depth, ToScale->Bpp, ToScale->Bpc) == IL_FALSE) {
		ilCloseImage(Scaled);
		if (ToScale != Image)
			ilCloseImage(ToScale);
		ilSetCurImage(CurImage);
		return NULL;
	}
	
	if (Height <= 1 && Image->Height <= 1) {
		iluScale1D_(ToScale, Scaled, Width);
	}
	if (Depth <= 1 && Image->Depth <= 1) {
		iluScale2D_(ToScale, Scaled, Width, Height);
	}
	else {
		iluScale3D_(ToScale, Scaled, Width, Height, Depth);
	}

	if (Format == IL_COLOUR_INDEX) {
		//ilSetCurImage(Scaled);
		//ilConvertImage(IL_COLOUR_INDEX);
		ilSetCurImage(CurImage);
		ilCloseImage(ToScale);
	}

	return Scaled;
}


ILimage *iluScale1D_(ILimage *Image, ILimage *Scaled, ILuint Width)
{
	ILuint		x1, x2;
	ILuint		NewX1, NewX2, NewX3, x, c;
	ILdouble	ScaleX, t1, t2, f;
	ILushort	*ShortPtr, *SShortPtr;
	ILuint		*IntPtr, *SIntPtr;

	if (Image == NULL) {
		iSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ScaleX = (ILdouble)Width / Image->Width;

	ShortPtr = (ILushort*)Image->Data;
	SShortPtr = (ILushort*)Scaled->Data;
	IntPtr = (ILuint*)Image->Data;
	SIntPtr = (ILuint*)Scaled->Data;

	if (iluFilter == ILU_NEAREST) {
		switch (Image->Bpc)
		{
			case 1:
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						Scaled->Data[NewX1 + c] = Image->Data[NewX2 + c];
					}
				}
				break;
			case 2:
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SShortPtr[NewX1 + c] = ShortPtr[NewX2 + c];
					}
				}
				break;
			case 4:
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SIntPtr[NewX1 + c] = IntPtr[NewX2 + c];
					}
				}
				break;
		}
	}
	else {  // IL_LINEAR or IL_BILINEAR
		switch (Image->Bpc)
		{
			case 1:
				NewX3 = 0;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t2 = t1 * Width - (ILuint)(t1 * Width);
					f = (1.0 - cos(t2 * IL_PI)) * .5;
					NewX1 = ((ILuint)(t1 * Width / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * Image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = Image->Data[NewX1 + c];
						x2 = Image->Data[NewX2 + c];

						Scaled->Data[NewX3 + c] = (ILubyte)(x1 * (1.0 - f) + x2 * f);
					}

					NewX3 += Scaled->Bpp;
				}
				break;
			case 2:
				NewX3 = 0;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t2 = t1 * Width - (ILuint)(t1 * Width);
					f = (1.0 - cos(t2 * IL_PI)) * .5;
					NewX1 = ((ILuint)(t1 * Width / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * Image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = ShortPtr[NewX1 + c];
						x2 = ShortPtr[NewX2 + c];

						SShortPtr[NewX3 + c] = (ILushort)(x1 * (1.0 - f) + x2 * f);
					}

					NewX3 += Scaled->Bpp;
				}
				break;
			case 4:
				NewX3 = 0;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t2 = t1 * Width - (ILuint)(t1 * Width);
					f = (1.0 - cos(t2 * IL_PI)) * .5;
					NewX1 = ((ILuint)(t1 * Width / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t1 * Width / ScaleX) + 1) * Image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = IntPtr[NewX1 + c];
						x2 = IntPtr[NewX2 + c];

						SIntPtr[NewX3 + c] = (ILuint)(x1 * (1.0 - f) + x2 * f);
					}

					NewX3 += Scaled->Bpp;
				}
				break;
		}
	}

	return Scaled;
}
