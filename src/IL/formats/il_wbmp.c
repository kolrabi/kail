//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_wbmp.c
//
// Description: Reads from a Wireless Bitmap (.wbmp) file.  Specs available from
//				http://www.ibm.com/developerworks/wireless/library/wi-wbmp/
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_WBMP
#include "il_bits.h"

// In il_quantizer.c
ILimage *iQuantizeImage(ILimage *Image, ILuint NumCols);
// In il_neuquant.c
ILimage *iNeuQuant(ILimage *Image, ILuint NumCols);

static ILuint WbmpGetMultibyte(SIO* io) {
	ILuint Val = 0, i;
	ILubyte Cur;

	for (i = 0; i < 5; i++) {  // Should not be more than 5 bytes.
		Cur = io->getchar(io->handle);
		Val = (Val << 7) | (Cur & 0x7F);  // Drop the MSB of Cur.
		if (!(Cur & 0x80)) {  // Check the MSB and break if 0.
			break;
		}
	}

	return Val;
}

static ILboolean iIsValidWbmp(SIO *io) {
	ILuint 		Start 	= SIOtell(io);
	ILushort  Head  	= GetLittleUShort(io);
	ILuint 	 	Width 	= WbmpGetMultibyte(io);
	ILuint    Height 	= WbmpGetMultibyte(io);
	ILboolean Eof     = SIOeof(io);

	SIOseek(io, Start, IL_SEEK_SET);

	return Head == 0 && Width != 0 && Height != 0 && !Eof;
}

static ILboolean iLoadWbmpInternal(ILimage *Image)
{
	ILuint	Width, Height, BitPadding, i;
	BITFILE	*File;
	ILubyte	Padding[8];
	SIO* io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;
	
	if (SIOgetc(io) != 0 || SIOgetc(io) != 0) {  // The first two bytes have to be 0 (the "header")
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	Width  = WbmpGetMultibyte(io);  // Next follows the width and height.
	Height = WbmpGetMultibyte(io);

	if (Width == 0 || Height == 0) {  // Must have at least some height and width.
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!iTexImage(Image, Width, Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	Image->Origin = IL_ORIGIN_UPPER_LEFT;  // Always has origin in the upper left.

	BitPadding = (8 - (Width % 8)) % 8;  // Has to be aligned on a byte boundary.  The rest is padding.
	File = bitfile(io);
	if (File == NULL)
		return IL_FALSE;  //@TODO: Error?

	//@TODO: Do this without bread?  Would be faster, since we would not have to do
	//  the second loop.

	// Reads the bits
	for (i = 0; i < Image->Height; i++) {
		bread(&Image->Data[Image->Width * i], 1, Image->Width, File);
		//bseek(File, BitPadding, IL_SEEK_CUR);  //@TODO: This function does not work correctly.
		bread(Padding, 1, BitPadding, File);  // Skip padding bits.
	}
	// Converts bit value of 1 to white and leaves 0 at 0 (2-colour images only).
	for (i = 0; i < Image->SizeOfData; i++) {
		if (Image->Data[i] == 1)
			Image->Data[i] = 0xFF;  // White
	}

	bclose(File);

	return IL_TRUE;
}


static ILboolean WbmpPutMultibyte(SIO* io, ILuint Val) {
	ILint	i, NumBytes = 0;
	ILuint	MultiVal = Val;

	do {
		MultiVal >>= 7;
		NumBytes++;
	} while (MultiVal != 0);

	for (i = NumBytes - 1; i >= 0; i--) {
		MultiVal = (Val >> (i * 7)) & 0x7F;
		if (i != 0)
			MultiVal |= 0x80;
		io->putchar(MultiVal, io->handle);
	}

	return IL_TRUE;
}

// Internal function used to save the Wbmp.
static ILboolean iSaveWbmpInternal(ILimage *Image) {
	ILimage	*TempImage = NULL;
	ILuint	i, j;
	ILint	k;
	ILubyte	Val;
	ILubyte	*TempData;
	SIO* io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;
	
	SIOputc(io, 0);  // First two header values
	SIOputc(io, 0);  //  must be 0.

	WbmpPutMultibyte(io, Image->Width);  // Write the width
	WbmpPutMultibyte(io, Image->Height); //  and the height.

	if (iGetInt(IL_QUANTIZATION_MODE) == IL_NEU_QUANT)
		TempImage = iNeuQuant(Image, 2);
	else // Assume IL_WU_QUANT otherwise.
		TempImage = iQuantizeImage(Image, 2);

	if (TempImage == NULL)
		return IL_FALSE;

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	} else {
		TempData = TempImage->Data;
	}

	for (i = 0; i < TempImage->Height; i++) {
		for (j = 0; j < TempImage->Width; j += 8) {
			Val = 0;
			for (k = 0; k < 8; k++) {
				if (j + k < TempImage->Width) {
					//Val |= ((TempData[TempImage->Width * i + j + k] > 0x7F) ? (0x80 >> k) : 0x00);
					Val |= ((TempData[TempImage->Width * i + j + k] == 1) ? (0x80 >> k) : 0x00);
				}
			}
			SIOputc(io, Val);
		}
	}

	if (TempData != TempImage->Data)
		ifree(TempData);

	ilCloseImage(TempImage);

	return IL_TRUE;
}

ILconst_string iFormatExtsWBMP[] = { 
	IL_TEXT("wbmp"), 
	NULL 
};

ILformat iFormatWBMP = { 
	/* .Validate = */ iIsValidWbmp, 
	/* .Load     = */ iLoadWbmpInternal, 
	/* .Save     = */ iSaveWbmpInternal, 
	/* .Exts     = */ iFormatExtsWBMP
};

#endif//IL_NO_WBMP

