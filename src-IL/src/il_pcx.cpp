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
ILuint iGetPcxHead(SIO* io, PCXHEAD *header)
{
	auto read = io->read(io->handle, header, 1, sizeof(PCXHEAD));

	#ifdef __BIG_ENDIAN__
	iSwapUShort(header->Xmin); // = GetLittleShort();
	iSwapUShort(header->Ymin); // = GetLittleShort();
	iSwapUShort(header->Xmax); // = GetLittleShort();
	iSwapUShort(header->Ymax); // = GetLittleShort();
	iSwapUShort(header->HDpi); // = GetLittleShort();
	iSwapUShort(header->VDpi); // = GetLittleShort();
	iSwapUShort(header->Bps); // = GetLittleShort();
	iSwapUShort(header->PaletteInfo); // = GetLittleShort();
	iSwapUShort(header->HScreenSize); // = GetLittleShort();
	iSwapUShort(header->VScreenSize); // = GetLittleShort();
	#endif

	return read;
}


// Check whether given file has a valid .pcx header
ILboolean iIsValidPcx(SIO* io)
{
	PCXHEAD Head;
	ILint read = iGetPcxHead(io, &Head);
	io->seek(io->handle, -read, IL_SEEK_CUR);

	if (read == sizeof(Head))
		return iCheckPcx(&Head);
	else
		return IL_FALSE;
}


// Internal function used to check if the HEADER is a valid .pcx header.
// Should we also do a check on Header->Bpp?
ILboolean iCheckPcx(PCXHEAD *Header)
{
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
ILboolean iLoadPcxInternal(ILimage* image)
{
	PCXHEAD	Header;
	ILboolean bPcx = IL_FALSE;

	if (image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return bPcx;
	}

	if (!iGetPcxHead(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckPcx(&Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	bPcx = iUncompressPcx(image, &Header);

	if (!bPcx)
		return IL_FALSE;
	return ilFixImage();
}


// Internal function to uncompress the .pcx (all .pcx files are rle compressed)
ILboolean iUncompressPcx(ILimage* image, PCXHEAD *Header)
{
	//changed decompression loop 2003-09-01
	//From the pcx spec: "There should always
	//be a decoding break at the end of each scan line.
	//But there will not be a decoding break at the end of
	//each plane within each scan line."
	//This is now handled correctly (hopefully ;) )

	ILubyte	ByteHead, Colour, *ScanLine /* For all planes */;
	ILuint ScanLineSize;
	ILuint	c, i, x, y;

	if (Header->Bpp < 8) {
		/*ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;*/
		return iUncompressSmall(image, Header);
	}

	if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (image->Bpp)
	{
		case 1:
			image->Format = IL_COLOUR_INDEX;
			image->Pal.PalType = IL_PAL_RGB24;
			image->Pal.PalSize = 256 * 3;  // Need to find out for sure...
			image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);
			if (image->Pal.Palette == NULL) {
				return IL_FALSE;
			}
			break;
		//case 2:  // No 16-bit images in the pcx format!
		case 3:
			image->Format = IL_RGB;
			image->Pal.Palette = NULL;
			image->Pal.PalSize = 0;
			image->Pal.PalType = IL_PAL_NONE;
			break;
		case 4:
			image->Format = IL_RGBA;
			image->Pal.Palette = NULL;
			image->Pal.PalSize = 0;
			image->Pal.PalType = IL_PAL_NONE;
			break;

		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	ScanLineSize = image->Bpp*Header->Bps;
	ScanLine = (ILubyte*)ialloc(ScanLineSize);
	if (ScanLine == NULL) {
		return IL_FALSE;
	}

	for (y = 0; y < image->Height; y++) {
		x = 0;
		//read scanline
		while (x < ScanLineSize) {
			if (image->io.read(image->io.handle, &ByteHead, 1, 1) != 1) {
				goto file_read_error;
			}
			if ((ByteHead & 0xC0) == 0xC0) {
				ByteHead &= 0x3F;
				if (image->io.read(image->io.handle, &Colour, 1, 1) != 1) {
					goto file_read_error;
				}
				if (x + ByteHead > ScanLineSize) {
					goto file_read_error;
				}
				for (i = 0; i < ByteHead; i++) {
					ScanLine[x++] = Colour;
				}
			}
			else {
				ScanLine[x++] = ByteHead;
			}
		}

		//convert plane-separated scanline into index, rgb or rgba pixels.
		//there might be a padding byte at the end of each scanline...
		for (x = 0; x < image->Width; x++) {
			for(c = 0; c < image->Bpp; c++) {
				image->Data[y * image->Bps + x * image->Bpp + c] =
						ScanLine[x + c * Header->Bps];
			}
		}
	}

	// Read in the palette
	if (Header->Version == 5 && image->Bpp == 1) {
		x = image->io.tell(image->io.handle);
		if (image->io.read(image->io.handle, &ByteHead, 1, 1) == 0) {  // If true, assume that we have a luminance image.
			ilGetError();  // Get rid of the IL_FILE_READ_ERROR.
			image->Format = IL_LUMINANCE;
			if (image->Pal.Palette)
				ifree(image->Pal.Palette);
			image->Pal.PalSize = 0;
			image->Pal.PalType = IL_PAL_NONE;
		}
		else {
			if (ByteHead != 12)  // Some Quake2 .pcx files don't have this byte for some reason.
				image->io.seek(image->io.handle, -1, IL_SEEK_CUR);
			if (image->io.read(image->io.handle, image->Pal.Palette, 1, image->Pal.PalSize) != image->Pal.PalSize)
				goto file_read_error;
		}
	}

	ifree(ScanLine);

	return IL_TRUE;

file_read_error:
	ifree(ScanLine);

	//added 2003-09-01
	ilSetError(IL_FILE_READ_ERROR);
	return IL_FALSE;
}


ILboolean iUncompressSmall(ILimage* image, PCXHEAD *Header)
{
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine;

	if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, 1, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (Header->NumPlanes)
	{
		case 1:
			image->Format = IL_LUMINANCE;
			break;
		case 4:
			image->Format = IL_COLOUR_INDEX;
			break;
		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	if (Header->NumPlanes == 1 && Header->Bpp == 1) {
		for (j = 0; j < image->Height; j++) {
			i = 0; //number of written pixels
			Bps = 0;
			while (Bps<Header->Bps) {
				if (image->io.read(image->io.handle, &HeadByte, 1, 1) != 1)
					return IL_FALSE;
				++Bps;
				// Check if we got duplicates with RLE compression
				if (HeadByte >= 192) {
					HeadByte -= 192;
					if (image->io.read(image->io.handle, &Data, 1, 1) != 1)
						return IL_FALSE;

					--Bps;
					// duplicate next byte
					for (c = 0; c < HeadByte; c++) {
						k = 128;
						for (d = 0; d < 8 && i < image->Width; d++) {
							image->Data[j * image->Width + i++] = ((Data & k) != 0 ? 255 : 0);
							k >>= 1;
						}
						++Bps;
					}
				}
				else {
					k = 128;
					for (c = 0; c < 8 && i < image->Width; c++) {
						image->Data[j * image->Width + i++] = ((HeadByte & k) != 0 ? 255 : 0);
						k >>= 1;
					}
				}
			}

			//if(Data != 0)
			//changed 2003-09-01:
			//There has to be an even number of bytes per line in a pcx.
			//One byte can hold up to 8 bits, so Width/8 bytes
			//are needed to hold a 1 bit per pixel image line.
			//If Width/8 is even no padding is needed,
			//one pad byte has to be read otherwise.
			//(let's hope the above is true ;-))

			// changed 2012-05-06
			// Not the good size - don't need it, padding inside data already !

		//	if(!((image->Width >> 3) & 0x1))
		//		image->io.getc(image->io.handle);	// Skip pad byte
		}
	}
	else if (Header->NumPlanes == 4 && Header->Bpp == 1){   // 4-bit images
		//changed decoding 2003-09-10 (was buggy)...could need a speedup

		Bps = Header->Bps * Header->NumPlanes * 8;
		image->Pal.Palette = (ILubyte*)ialloc(16 * 3);  // Size of palette always (48 bytes).
		ScanLine = (ILubyte*)ialloc(Bps);
		if (image->Pal.Palette == NULL || ScanLine == NULL) {
			ifree(ScanLine);
			ifree(image->Pal.Palette);
			return IL_FALSE;
		}
		memcpy(image->Pal.Palette, Header->ColMap, 16 * 3);
		image->Pal.PalSize = 16 * 3;
		image->Pal.PalType = IL_PAL_RGB24;

		memset(image->Data, 0, image->SizeOfData);

		for (y = 0; y < image->Height; y++) {
			x = 0;
			while (x < Bps) {
				if (image->io.read(image->io.handle, &HeadByte, 1, 1) != 1) {
					ifree(ScanLine);
					return IL_FALSE;
				}
				if ((HeadByte & 0xC0) == 0xC0) {
					HeadByte &= 0x3F;
					if (image->io.read(image->io.handle, &Colour, 1, 1) != 1) {
						ifree(ScanLine);
						return IL_FALSE;
					}
					for (i = 0; i < HeadByte; i++) {
						k = 128;
						for (j = 0; j < 8 && x < Bps; j++) {
							ScanLine[x++] = (Colour & k)?1:0;
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (j = 0; j < 8 && x < Bps; j++) {
						ScanLine[x++] = (HeadByte & k)?1:0;
						k >>= 1;
					}
				}
			}

			for (x = 0; x < image->Width; x++) {  // 'Cleverly' ignores the pad bytes. ;)
				for(c = 0; c < Header->NumPlanes; c++)
					image->Data[y * image->Width + x] |= ScanLine[x + c*Header->Bps*8] << c;
			}
		}
		ifree(ScanLine);
	}
	else {
		ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	return IL_TRUE;
}

// Routine used from ZSoft's pcx documentation
ILuint encput(SIO* io, ILubyte byt, ILubyte cnt)
{
	if (cnt) {
		if ((cnt == 1) && (0xC0 != (0xC0 & byt))) {
			if (IL_EOF == io->putc(byt, io->handle))
				return(0);     /* disk write error (probably full) */
			return(1);
		}
		else {
			if (IL_EOF == io->putc((ILubyte)((ILuint)0xC0 | cnt), io->handle))
				return (0);      /* disk write error */
			if (IL_EOF == io->putc(byt, io->handle))
				return (0);      /* disk write error */
			return (2);
		}
	}

	return (0);
}


// This subroutine encodes one scanline and writes it to a file.
//  It returns number of bytes written into outBuff, 0 if failed.
ILuint encLine(SIO* io, ILubyte *inBuff, ILint inLen, ILubyte Stride)
{
	ILubyte _this, last;
	ILint srcIndex, i;
	ILint total;
	ILubyte runCount;     // max single runlength is 63
	total = 0;
	runCount = 1;
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
				if (! (i = encput(io, last, runCount)))
						return (0);
				total += i;
				runCount = 0;
			}
		}
		else {  // No "run"  -  _this != last
			if (runCount) {
				if (! (i = encput(io, last, runCount)))
					return(0);
				total += i;
			}
			last = _this;
			runCount = 1;
		}
	}  // endloop

	if (runCount) {  // finish up
		if (! (i = encput(io, last, runCount)))
			return (0);
		if (inLen % 2)
			io->putc(0, io->handle);
		return (total + i);
	}
	else {
		if (inLen % 2)
			io->putc(0, io->handle);
	}

	return (total);
}


// Internal function used to save the .pcx.
ILboolean iSavePcxInternal(ILimage* image)
{
	ILuint	i, c, PalSize;
	ILpal	*TempPal;
	ILimage	*TempImage = image;
	ILubyte	*TempData;

	if (image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

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
				ilCloseImage(TempImage);
			}
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}


	image->io.putc(0xA, image->io.handle);  // Manufacturer - always 10
	image->io.putc(0x5, image->io.handle);  // Version Number - always 5
	image->io.putc(0x1, image->io.handle);  // Encoding - always 1
	image->io.putc(0x8, image->io.handle);  // Bits per channel
	SaveLittleUShort(&image->io, 0);  // X Minimum
	SaveLittleUShort(&image->io, 0);  // Y Minimum
	SaveLittleUShort(&image->io, (ILushort)(image->Width - 1));
	SaveLittleUShort(&image->io, (ILushort)(image->Height - 1));
	SaveLittleUShort(&image->io, 0);
	SaveLittleUShort(&image->io, 0);

	// Useless palette info?
	for (i = 0; i < 48; i++) {
		image->io.putc(0, image->io.handle);
	}
	image->io.putc(0x0, image->io.handle);  // Reserved - always 0

	image->io.putc(image->Bpp, image->io.handle);  // Number of planes - only 1 is supported right now

	SaveLittleUShort(&image->io, (ILushort)(image->Width & 1 ? image->Width + 1 : image->Width));  // Bps
	SaveLittleUShort(&image->io, 0x1);  // Palette type - ignored?

	// Mainly filler info
	for (i = 0; i < 58; i++) {
		image->io.putc(0x0, image->io.handle);
	}

	// Output data
	for (i = 0; i < TempImage->Height; i++) {
		for (c = 0; c < TempImage->Bpp; c++) {
			encLine(&image->io, TempData + TempImage->Bps * i + c, TempImage->Width, (ILubyte)(TempImage->Bpp - 1));
		}
	}

	// Automatically assuming we have a palette...dangerous!
	//	Also assuming 3 bpp palette
	image->io.putc(0xC, image->io.handle);  // Pad byte must have this value

	// If the current image has a palette, take care of it
	if (TempImage->Format == IL_COLOUR_INDEX) {
		// If the palette in .pcx format, write it directly
		if (TempImage->Pal.PalType == IL_PAL_RGB24) {
			image->io.write(TempImage->Pal.Palette, 1, TempImage->Pal.PalSize, image->io.handle);
		}
		else {
			TempPal = iConvertPal(&TempImage->Pal, IL_PAL_RGB24);
			if (TempPal == NULL) {
				if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
					ifree(TempData);
				if (TempImage != image)
					ilCloseImage(TempImage);
				return IL_FALSE;
			}

			image->io.write(TempPal->Palette, 1, TempPal->PalSize, image->io.handle);
			ifree(TempPal->Palette);
			ifree(TempPal);
		}
	}

	// If the palette is not all 256 colours, we have to pad it.
	PalSize = 768 - image->Pal.PalSize;
	for (i = 0; i < PalSize; i++) {
		image->io.putc(0x0, image->io.handle);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != image)
		ilCloseImage(TempImage);

	return IL_TRUE;
}


#endif//IL_NO_PCX
