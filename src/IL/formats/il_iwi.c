//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/04/2009
//
// Filename: src-IL/src/il_iwi.c
//
// Description: Reads from an Infinity Ward Image (.iwi) file from Call of Duty.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_IWI
#include "il_dds.h"

#include "pack_push.h"
typedef struct IWIHEAD
{
	ILuint		Signature;
	ILubyte		Format;
	ILubyte		Flags;					// TODO: find out meaning of flags
	ILushort	Width;
	ILushort	Height;
	ILubyte   Unknown[18];		// TODO: find out meaning of these bytes
} IWIHEAD;
#include "pack_pop.h"

#define IWI_ARGB8	0x01
#define IWI_RGB8	0x02
#define IWI_ARGB4	0x03
#define IWI_A8		0x04
#define IWI_JPG		0x07
#define IWI_DXT1	0x0B
#define IWI_DXT3	0x0C
#define IWI_DXT5	0x0D

static ILboolean iCheckIwi(IWIHEAD *Header);
static ILboolean iLoadIwiInternal(ILimage *Image);
static ILboolean IwiInitMipmaps(ILimage *BaseImage, ILuint *NumMips);
static ILboolean IwiReadImage(ILimage *BaseImage, IWIHEAD *Header, ILuint NumMips);
static ILenum    IwiGetFormat(ILubyte Format, ILubyte *Bpp);

static ILboolean iIsValidIwi(SIO *io) {
	ILuint		FirstPos = SIOtell(io);
	ILubyte   Magic[4];
	ILuint    Read     = SIOread(io, Magic, 1, 4);

	SIOseek(io, FirstPos, IL_SEEK_SET);

	return Read == 4 && (!memcmp(Magic, "IWi\x05", 4) || !memcmp(Magic, "IWi\x06", 4));
}

// Internal function used to get the IWI header from the current file.
static ILboolean iGetIwiHead(SIO *io, IWIHEAD *Header) {
	if (!SIOread(io, Header, sizeof(*Header), 1))
		return IL_FALSE;

	UInt  (&Header->Signature);
	UShort(&Header->Width);
	UShort(&Header->Height);

	return IL_TRUE;
}

// Internal function used to check if the HEADER is a valid IWI header.
ILboolean iCheckIwi(IWIHEAD *Header) {
	if ( Header->Signature != 0x06695749 
	  && Header->Signature != 0x05695749 )  // 'IWi-' (version 6, and version 5 is the second).
		return IL_FALSE;

	if ( Header->Width  == 0 
		|| Header->Height == 0 )
		return IL_FALSE;

	// DXT images must have power-of-2 dimensions.
	if ( Header->Format == IWI_DXT1 
	  || Header->Format == IWI_DXT3  
	  || Header->Format == IWI_DXT5 )
		if ( Header->Width  != ilNextPower2(Header->Width) 
			|| Header->Height != ilNextPower2(Header->Height))
			return IL_FALSE;

	// Format must be valid
	if ( Header->Format != IWI_ARGB4 
		&& Header->Format != IWI_RGB8  
		&& Header->Format != IWI_ARGB8 
		&& Header->Format != IWI_A8 
		&& Header->Format != IWI_DXT1 
		&& Header->Format != IWI_DXT3 
		&& Header->Format != IWI_DXT5 )
		return IL_FALSE;

	return IL_TRUE;
}

// Internal function used to load the IWI.
static ILboolean iLoadIwiInternal(ILimage *Image) {
	IWIHEAD		Header;
	ILuint		NumMips = 0;
	ILboolean	HasMipmaps = IL_TRUE;
	ILenum		Format;
	ILubyte		Bpp;
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	// Read the header and check it.
	if (!iGetIwiHead(io, &Header))
		return IL_FALSE;

	if (!iCheckIwi(&Header)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// From a post by Pointy on http://iwnation.com/forums/index.php?showtopic=27903,
	// flags ending with 0x3 have no mipmaps.
	HasMipmaps = ((Header.Flags & 0x03) == 0x03) ? IL_FALSE : IL_TRUE;

	// Create the image, then create the mipmaps, then finally read the image.
	Format = IwiGetFormat(Header.Format, &Bpp);
	
	if (!iTexImage(Image, Header.Width, Header.Height, 1, Bpp, Format, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	if (HasMipmaps && !IwiInitMipmaps(Image, &NumMips))
			return IL_FALSE;

	if (!IwiReadImage(Image, &Header, NumMips))
		return IL_FALSE;

	return IL_TRUE;
}

// Helper function to convert IWI formats to DevIL formats and Bpp.
static ILenum IwiGetFormat(ILubyte Format, ILubyte *Bpp) {
	switch (Format)	{
		case IWI_ARGB8:
			*Bpp = 4;
			return IL_BGRA;
		case IWI_RGB8:
			*Bpp = 3;
			return IL_BGR;
		case IWI_ARGB4:
			*Bpp = 4;
			return IL_BGRA;
		case IWI_A8:
			*Bpp = 1;
			return IL_ALPHA;
		case IWI_DXT1:
			*Bpp = 4;
			return IL_RGBA;
		case IWI_DXT3:
			*Bpp = 4;
			return IL_RGBA;
		case IWI_DXT5:
			*Bpp = 4;
			return IL_RGBA;
	}

	return 0;  // Will never reach this.
}


// Function to intialize the mipmaps and determine the number of mipmaps.
static ILboolean IwiInitMipmaps(ILimage *BaseImage, ILuint *NumMips)
{
	ILimage	*Image;
	ILuint	Width, Height, Mipmap;

	Image = BaseImage;
	Width = BaseImage->Width;  Height = BaseImage->Height;
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	for (Mipmap = 0; Width != 1 && Height != 1; Mipmap++) {
		// 1 is the smallest dimension possible.
		Width  = (Width  >> 1) == 0 ? 1 : (Width >> 1);
		Height = (Height >> 1) == 0 ? 1 : (Height >> 1);

		Image->Mipmaps = ilNewImageFull(Width, Height, 1, BaseImage->Bpp, BaseImage->Format, BaseImage->Type, NULL);
		if (Image->Mipmaps == NULL)
			return IL_FALSE;
		Image = Image->Mipmaps;

		// ilNewImage does not set these.
		/* ilNewImageFull does...
		Image->Format = BaseImage->Format;
		Image->Type   = BaseImage->Type;
		*/

		// The origin is in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	*NumMips = Mipmap;
	return IL_TRUE;
}

static ILboolean IwiReadImage(ILimage *BaseImage, IWIHEAD *Header, ILuint NumMips) {
	ILimage	*Image;
	ILuint	SizeOfData;
	ILubyte	*CompData = NULL;
	ILint	i, j, k, m;

	SIO *io = &BaseImage->io;

	for (i = NumMips; i >= 0; i--) {
		Image = BaseImage;
		// Go to the ith mipmap level.
		//  The mipmaps go from smallest to the largest.
		for (j = 0; j < i; j++)
			Image = Image->Mipmaps;

		switch (Header->Format)
		{
			case IWI_ARGB8: // These are all
			case IWI_RGB8:  //  uncompressed data,
			case IWI_A8:    //  so just read it.
				if (SIOread(io, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData)
					return IL_FALSE;
				break;

			case IWI_ARGB4:  //@TODO: Find some test images for this.
				// Data is in ARGB4 format - 4 bits per component.
				SizeOfData = Image->Width * Image->Height * 2;
				CompData   = ialloc(SizeOfData);  // Not really compressed - just in ARGB4 format.

				if (CompData == NULL)
					return IL_FALSE;

				if (SIOread(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}
				for (k = 0, m = 0; k < (ILint)Image->SizeOfData; k += 4, m += 2) {
					// @TODO: Double the image data into the low and high nibbles for a better range of values.
					Image->Data[k+0] =  CompData[m  ] & 0xF0;
					Image->Data[k+1] = (CompData[m  ] & 0x0F) << 4;
					Image->Data[k+2] =  CompData[m+1] & 0xF0;
					Image->Data[k+3] = (CompData[m+1] & 0x0F) << 4;
				}
				break;

			case IWI_DXT1:
				// DXT1 data has at least 8 bytes, even for one pixel.
				SizeOfData = IL_MAX(Image->Width * Image->Height / 2, 8);
				CompData   = ialloc(SizeOfData);  // Gives a 6:1 compression ratio (or 8:1 for DXT1 with alpha)

				if (CompData == NULL)
					return IL_FALSE;

				if (SIOread(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Decompress the DXT1 data into Image (ith mipmap).
				if (!DecompressDXT1(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Keep a copy of the DXTC data if the user wants it.
				if (ilGetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
					Image->DxtcSize = SizeOfData;
					Image->DxtcData = CompData;
					Image->DxtcFormat = IL_DXT1;
					CompData = NULL;
				}

				break;

			case IWI_DXT3:
				// DXT3 data has at least 16 bytes, even for one pixel.
				SizeOfData = IL_MAX(Image->Width * Image->Height, 16);
				CompData   = ialloc(SizeOfData);  // Gives a 4:1 compression ratio

				if (CompData == NULL)
					return IL_FALSE;

				if (SIOread(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Decompress the DXT3 data into Image (ith mipmap).
				if (!DecompressDXT3(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}
				break;

			case IWI_DXT5:
				// DXT5 data has at least 16 bytes, even for one pixel.
				SizeOfData = IL_MAX(Image->Width * Image->Height, 16);
				CompData   = ialloc(SizeOfData);  // Gives a 4:1 compression ratio

				if (CompData == NULL)
					return IL_FALSE;

				if (SIOread(io, CompData, 1, SizeOfData) != SizeOfData) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Decompress the DXT5 data into Image (ith mipmap).
				if (!DecompressDXT5(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}
				break;
		}
	
		ifree(CompData);
	}

	return IL_TRUE;
}


ILconst_string iFormatExtsIWI[] = { 
  IL_TEXT("iwi"), 
  NULL 
};

ILformat iFormatIWI = { 
  /* .Validate = */ iIsValidIwi, 
  /* .Load     = */ iLoadIwiInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsIWI
};

#endif//IL_NO_IWI
