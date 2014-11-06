//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pcx.c
//
// Description: Reads and writes from/to a .pcx file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PCX
#include "il_pcx.h"
#include "il_manip.h"

// Obtain .pcx header
ILboolean PcxGetHeader(SIO* io, PCXHEAD *header) {
	ILuint read = SIOread(io, header, 1, sizeof(*header));
	if (read != sizeof(*header))
		return IL_FALSE;

	UShort(&header->Xmin);
	UShort(&header->Ymin);
	UShort(&header->Xmax);
	UShort(&header->Ymax);
	UShort(&header->HDpi);
	UShort(&header->VDpi);
	UShort(&header->Bps);
	UShort(&header->PaletteInfo);
	UShort(&header->HScreenSize);
	UShort(&header->VScreenSize); 

	return IL_TRUE;
}

// Check whether given file has a valid .pcx header
static ILboolean iIsValidPcx(SIO* io) {
	ILuint 			Pos = SIOtell(io);
	PCXHEAD 		Head;
	ILboolean 	HasHead = PcxGetHeader(io, &Head);

	SIOseek(io, Pos, IL_SEEK_SET);

	return HasHead && PcxCheckHeader(&Head);
}

// Internal function used to check if the HEADER is a valid .pcx header.
// Should we also do a check on Header->Bpp?
ILboolean PcxCheckHeader(PCXHEAD *Header) {
	//	Got rid of the Reserved check, because I've seen some .pcx files with invalid values in it.
	if (Header->Manufacturer != 10 || Header->Encoding != 1/* || Header->Reserved != 0*/)
		return IL_FALSE;

	// Try to support all pcx versions, as they only differ in allowed formats...
	// Let's hope it works.
	if(Header->Version != 5 && Header->Version != 0 && Header->Version != 2 &&
		 Header->VDpi != 3 && Header->VDpi != 4)
		return IL_FALSE;

	// See if the padding size is correct
	// 2014-03-27: gimp generates files that violate this property, but they are correctly decoded
	/*ILuint	Test = Header->Xmax - Header->Xmin + 1;
	if (Header->Bpp >= 8) {
		if (Test & 1) {
			if (Header->Bps != Test + 1)
				return IL_FALSE;
		}
		else {
			if (Header->Bps != Test)  // No padding
				return IL_FALSE;
		}
	}*/

	/* for (i = 0; i < 54; i++) { useless check
		if (Header->Filler[i] != 0)
			return IL_FALSE;
	} */

	return IL_TRUE;
}


// Internal function used to load the .pcx.
static ILboolean iLoadPcxInternal(ILimage* Image) {
	PCXHEAD	Header;
	ILboolean bPcx = IL_FALSE;
	ILimage *TempImage;
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return bPcx;
	}

  io = &Image->io;

	if (!PcxGetHeader(io, &Header))
		return IL_FALSE;

	if (!PcxCheckHeader(&Header)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	TempImage = PcxDecode(io, &Header);
	if (TempImage) {
		iTexImage(Image, TempImage->Width, TempImage->Height, 1, TempImage->Bpp, TempImage->Format, TempImage->Type, TempImage->Data);
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
		iCloseImage(TempImage);
		return IL_TRUE;
	}

	return IL_FALSE;
}

static ILboolean PcxDecodeScanline(SIO *io, ILubyte *ScanLine, ILuint ScanLineSize)
{
	ILuint x, i;
	ILubyte ByteHead, Colour;

	x = 0;

	//read scanline
	while (x < ScanLineSize) {
		if (SIOread(io, &ByteHead, 1, 1) != 1) {
			return IL_FALSE;
		}

		if ((ByteHead & 0xC0) == 0xC0) {
			ByteHead &= 0x3F;
			if (SIOread(io, &Colour, 1, 1) != 1) {
				return IL_FALSE;
			}

			if (x + ByteHead > ScanLineSize) {
				return IL_FALSE;
			}

			for (i = 0; i < ByteHead; i++) {
				ScanLine[x++] = Colour;
			}
		}	else {
			ScanLine[x++] = ByteHead;
		}
	}

	return IL_TRUE;
}

static ILimage *
PcxDecodeSmall(SIO *io, PCXHEAD *Header) {
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine = NULL;
	ILimage	*Image;

	Image = iNewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (Header->NumPlanes)
	{
		case 1:
			Image->Format = IL_LUMINANCE;
			break;
		case 4:
			Image->Format = IL_COLOUR_INDEX;
			break;
		default:
			iSetError(IL_ILLEGAL_FILE_VALUE);
			iCloseImage(Image);
			return NULL;
	}

	if (Header->NumPlanes == 1) {
		for (j = 0; j < Image->Height; j++) {
			i = 0;
			while (i < Image->Width) {
				if (SIOread(io, &HeadByte, 1, 1) != 1)
					goto file_read_error;
				if (HeadByte >= 192) {
					HeadByte -= 192;
					if (SIOread(io, &Data, 1, 1) != 1)
						goto file_read_error;

					for (c = 0; c < HeadByte; c++) {
						k = 128;
						for (d = 0; d < 8 && i < Image->Width; d++) {
							Image->Data[j * Image->Width + i++] = (!!(Data & k) == 1 ? 255 : 0);
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (c = 0; c < 8 && i < Image->Width; c++) {
						Image->Data[j * Image->Width + i++] = (!!(HeadByte & k) == 1 ? 255 : 0);
						k >>= 1;
					}
				}
			}
			if (Data != 0)
				SIOgetc(io);  // Skip pad byte if last byte not a 0
		}
	}
	else {   // 4-bit images
		Bps = Header->Bps * Header->NumPlanes * 2;
		Image->Pal.Palette = (ILubyte*)ialloc(16 * 3);  // Size of palette always (48 bytes).
		Image->Pal.PalSize = 16 * 3;
		Image->Pal.PalType = IL_PAL_RGB24;
		ScanLine = (ILubyte*)ialloc(Bps);
		if (Image->Pal.Palette == NULL || ScanLine == NULL) {
			ifree(ScanLine);
			iCloseImage(Image);
			return NULL;
		}

		memcpy(Image->Pal.Palette, Header->ColMap, 16 * 3);
		imemclear(Image->Data, Image->SizeOfData);  // Since we do a += later.

		for (y = 0; y < Image->Height; y++) {
			for (c = 0; c < Header->NumPlanes; c++) {
				x = 0;
				while (x < Bps) {
					if (SIOread(io, &HeadByte, 1, 1) != 1)
						goto file_read_error;
					if ((HeadByte & 0xC0) == 0xC0) {
						HeadByte &= 0x3F;
						if (SIOread(io, &Colour, 1, 1) != 1)
							goto file_read_error;
						for (i = 0; i < HeadByte; i++) {
							k = 128;
							for (j = 0; j < 8; j++) {
								ScanLine[x++] = !!(Colour & k);
								k >>= 1;
							}
						}
					}
					else {
						k = 128;
						for (j = 0; j < 8; j++) {
							ScanLine[x++] = !!(HeadByte & k);
							k >>= 1;
						}
					}
				}

				for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes. ;)
					Image->Data[y * Image->Width + x] += ScanLine[x] << c;
				}
			}
		}
		ifree(ScanLine);
	}

	return Image;

file_read_error:
	ifree(ScanLine);
	iCloseImage(Image);
	return NULL;
}

// Internal function to uncompress the .dcx (all .dcx files are rle compressed)
ILimage *
PcxDecode(SIO *io, PCXHEAD *Header) {
	ILubyte		ByteHead, *ScanLine = NULL /* Only one plane */;
	ILuint		c, x, y, ScanLineSize;//, Read = 0;
	ILimage		*Image = NULL;

	if (Header->Bpp < 8) {
		return PcxDecodeSmall(io, Header);
	}

	Image = iNewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	ScanLineSize 	= Image->Bpp*Header->Bps;
	ScanLine = (ILubyte*)ialloc(ScanLineSize);
	if (ScanLine == NULL)
		goto dcx_error;

	switch (Image->Bpp)
	{
		case 1:
			Image->Format = IL_COLOUR_INDEX;
			Image->Pal.PalType = IL_PAL_RGB24;
			Image->Pal.PalSize = 256 * 3; // Need to find out for sure...
			Image->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
			if (Image->Pal.Palette == NULL)
				goto dcx_error;
			break;
		//case 2:  // No 16-bit images in the dcx format!
		case 3:
			Image->Format = IL_RGB;
			Image->Pal.Palette = NULL;
			Image->Pal.PalSize = 0;
			Image->Pal.PalType = IL_PAL_NONE;
			break;
		case 4:
			Image->Format = IL_RGBA;
			Image->Pal.Palette = NULL;
			Image->Pal.PalSize = 0;
			Image->Pal.PalType = IL_PAL_NONE;
			break;

		default:
			iSetError(IL_ILLEGAL_FILE_VALUE);
			goto dcx_error;
	}

	for (y = 0; y < Image->Height; y++) {
		if (!PcxDecodeScanline(io, ScanLine, ScanLineSize))
			goto dcx_error;

		//convert plane-separated scanline into index, rgb or rgba pixels.
		//there might be a padding byte at the end of each scanline...
		for (x = 0; x < Image->Width; x++) {
			for(c = 0; c < Image->Bpp; c++) {
				Image->Data[y * Image->Bps + x * Image->Bpp + c] =
						ScanLine[x + c * Header->Bps];
			}
		}
	}		

	ifree(ScanLine);

	// Read in the palette
	if (Image->Bpp == 1) {
		ByteHead = (ILubyte)SIOgetc(io);	// the value 12, because it signals there's a palette for some reason...

		// We should do a check to make certain it's 12...
		if (ByteHead != 12)
			SIOseek(io, -1, IL_SEEK_CUR);

		if (SIOread(io, Image->Pal.Palette, 1, Image->Pal.PalSize) != Image->Pal.PalSize) {
			iCloseImage(Image);
			return NULL;
		}
	}

	return Image;

dcx_error:
	ifree(ScanLine);
	iCloseImage(Image);
	return NULL;
}


// Routine used from ZSoft's pcx documentation
static ILuint PcxEncodeWrite(SIO* io, ILubyte byt, ILubyte cnt)
{
	if (cnt) {
		if ((cnt == 1) && (0xC0 != (0xC0 & byt))) {
			if (IL_EOF == SIOputc(io, byt))
				return(0);     /* disk write error (probably full) */
			return(1);
		}
		else {
			if (IL_EOF == SIOputc(io, (ILubyte)((ILuint)0xC0 | cnt)))
				return (0);      /* disk write error */
			if (IL_EOF == SIOputc(io, byt))
				return (0);      /* disk write error */
			return (2);
		}
	}

	return (0);
}


// This subroutine encodes one scanline and writes it to a file.
//  It returns number of bytes written into outBuff, 0 if failed.
static ILuint PcxEncodeScanline(SIO* io, ILubyte *inBuff, ILuint inLen, ILubyte Stride) {
	ILubyte _this, last;
	ILuint srcIndex;
	ILuint total = 0;
	ILuint i;
	ILubyte runCount = 1;     // max single runlength is 63

	last = *(inBuff);

	// Find the pixel dimensions of the image by calculating 
	//[XSIZE = Xmax - Xmin + 1] and [YSIZE = Ymax - Ymin + 1].  
	//Then calculate how many bytes are in a "run"

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		inBuff += Stride;
		_this = *(++inBuff);
		if (_this == last) {  // There is a "run" in the data, encode it
			runCount++;
			if (runCount == 63) {
				if (! (i = PcxEncodeWrite(io, last, runCount)))
						return (0);
				total += i;
				runCount = 0;
			}
		}
		else {  // No "run"  -  _this != last
			if (runCount) {
				if (! (i = PcxEncodeWrite(io, last, runCount)))
					return(0);
				total += i;
			}
			last = _this;
			runCount = 1;
		}
	}  // endloop

	if (runCount) {  // finish up
		if (! (i = PcxEncodeWrite(io, last, runCount)))
			return (0);
		if (inLen % 2)
			SIOputc(io, 0);
		return (total + i);
	}	else {
		if (inLen % 2)
			SIOputc(io, 0);
	}

	return (total);
}

ILboolean PcxEncode(SIO *io, ILimage *image)
{
	ILuint	i, c, PalSize;
	ILpal	*TempPal;
	ILimage	*TempImage = image;
	ILubyte	*TempData;

	switch (image->Format)
	{
		case IL_LUMINANCE:
			TempImage = iConvertImage(image, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		case IL_BGR:
			TempImage = iConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		case IL_BGRA:
			TempImage = iConvertImage(image, IL_RGBA, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		default:
			if (image->Bpc > 1) {
				TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
				if (TempImage == NULL)
					return IL_FALSE;
			}
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != image) {
				iCloseImage(TempImage);
			}
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}


	SIOputc(io, 0xA);  // Manufacturer - always 10
	SIOputc(io, 0x5);  // Version Number - always 5
	SIOputc(io, 0x1);  // Encoding - always 1
	SIOputc(io, 0x8);  // Bits per channel
	SaveLittleUShort(io, 0);  // X Minimum
	SaveLittleUShort(io, 0);  // Y Minimum
	SaveLittleUShort(io, (ILushort)(image->Width - 1));
	SaveLittleUShort(io, (ILushort)(image->Height - 1));
	SaveLittleUShort(io, 0);
	SaveLittleUShort(io, 0);

	// Useless palette info?
	for (i = 0; i < 48; i++) {
		SIOputc(io, 0);
	}
	SIOputc(io, 0x0);  // Reserved - always 0

	SIOputc(io, image->Bpp);  // Number of planes - only 1 is supported right now

	SaveLittleUShort(io, (ILushort)(image->Width & 1 ? image->Width + 1 : image->Width));  // Bps
	SaveLittleUShort(io, 0x1);  // Palette type - ignored?

	// Mainly filler info
	for (i = 0; i < 58; i++) {
		SIOputc(io, 0x0);
	}

	// Output data
	for (i = 0; i < TempImage->Height; i++) {
		for (c = 0; c < TempImage->Bpp; c++) {
			PcxEncodeScanline(io, TempData + TempImage->Bps * i + c, TempImage->Width, (ILubyte)(TempImage->Bpp - 1));
		}
	}

	// Automatically assuming we have a palette...dangerous!
	//	Also assuming 3 bpp palette
	SIOputc(io, 0xC);  // Pad byte must have this value

	// If the current image has a palette, take care of it
	if (TempImage->Format == IL_COLOUR_INDEX) {
		// If the palette in .pcx format, write it directly
		if (TempImage->Pal.PalType == IL_PAL_RGB24) {
			SIOwrite(io, TempImage->Pal.Palette, 1, TempImage->Pal.PalSize);
		} else {
			TempPal = iConvertPal(&TempImage->Pal, IL_PAL_RGB24);
			if (TempPal == NULL) {
				if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
					ifree(TempData);
				if (TempImage != image)
					iCloseImage(TempImage);
				return IL_FALSE;
			}

			SIOwrite(io, TempPal->Palette, 1, TempPal->PalSize);
			ifree(TempPal->Palette);
			ifree(TempPal);
		}
	}

	// If the palette is not all 256 colours, we have to pad it.
	PalSize = 768 - image->Pal.PalSize;
	for (i = 0; i < PalSize; i++) {
		SIOputc(io, 0x0);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != image)
		iCloseImage(TempImage);

	return IL_TRUE;
}

// Internal function used to save the .pcx.
static ILboolean iSavePcxInternal(ILimage* image) {
	SIO *io;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;

	return PcxEncode(io, image);
}


static ILconst_string iFormatExtsPCX[] = { 
  IL_TEXT("pcx"), 
  NULL 
};

ILformat iFormatPCX = { 
  /* .Validate = */ iIsValidPcx, 
  /* .Load     = */ iLoadPcxInternal, 
  /* .Save     = */ iSavePcxInternal, 
  /* .Exts     = */ iFormatExtsPCX
};

#endif//IL_NO_PCX
