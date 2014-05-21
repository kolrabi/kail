//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 2014-05-21 by Björn Paetzel
//
// Filename: src/IL/formats/il_dcx.c
//
// Description: Reads from a .dcx file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DCX
#include "il_dcx.h"
#include "il_manip.h"

static ILimage *iUncompressDcx(SIO *io, DCXHEAD *Header);
static ILimage *iUncompressDcxSmall(SIO *io, DCXHEAD *Header);

static ILboolean 
iIsValidDcx(SIO *io) {
	ILuint Signature;

	ILuint Read = SIOread(io, &Signature, 1, 4);
	SIOseek(io, -Read, IL_SEEK_CUR);

	if (Read != 4)
		return IL_FALSE;

	UInt(Signature);

	return (Signature == 987654321);
}

// Internal function obtain the .dcx header from the current file.
static ILboolean 
iGetDcxHead(SIO *io, DCXHEAD *Head) {
	if (SIOread(io, Head, 1, sizeof(*Head)) != sizeof(*Head))
		return IL_FALSE;

	UShort(Head->Xmin);
	UShort(Head->Ymin);
	UShort(Head->Xmax);
	UShort(Head->Ymax);
	UShort(Head->HDpi);
	UShort(Head->VDpi);
	UShort(Head->Bps);
	UShort(Head->PaletteInfo);
	UShort(Head->HScreenSize);
	UShort(Head->VScreenSize);

	return IL_TRUE;
}

#if 0 // seems unused
// Internal function used to check if the HEADER is a valid .dcx header.
// Should we also do a check on Header->Bpp?
static ILboolean 
iCheckDcx(DCXHEAD *Header) {
	ILuint	i;

	// There are other versions, but I am not supporting them as of yet.
	//	Got rid of the Reserved check, because I've seen some .dcx files with invalid values in it.
	if (Header->Manufacturer != 10 || Header->Version != 5 || Header->Encoding != 1/* || Header->Reserved != 0*/)
		return IL_FALSE;

	// See if the padding size is correct
	//ILuint Test = Header->Xmax - Header->Xmin + 1;
	/*if (Header->Bpp >= 8) {
		if (Test & 1) {
			if (Header->Bps != Test + 1)
				return IL_FALSE;
		}
		else {
			if (Header->Bps != Test)  // No padding
				return IL_FALSE;
		}
	}*/

	for (i = 0; i < 54; i++) {
		if (Header->Filler[i] != 0)
			return IL_FALSE;
	}

	return IL_TRUE;
}

#endif

// Internal function used to load the .dcx.
static ILboolean
iLoadDcxInternal(ILimage *TargetImage)
{
	DCXHEAD	Header;
	ILuint	Signature, i, Entries[1024], Num = 0;
	ILimage	*Prev = TargetImage;
	ILimage *Image = TargetImage;

	if (TargetImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &TargetImage->io;

	if (!iIsValidDcx(io))
		return IL_FALSE;

	SIOread(io, &Signature, 1, 4);

	do {
		if (SIOread(io, &Entries[Num], 1, 4) != 4)
			return IL_FALSE;
		UShort(Entries[Num]);
		Num++;
	} while (Entries[Num-1] != 0);

	for (i = 0; i < Num; i++) {
		SIOseek(io, Entries[i], IL_SEEK_SET);
		iGetDcxHead(io, &Header);
		/*if (!iCheckDcx(&Header)) {
			ilSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}*/

		Image = iUncompressDcx(io, &Header);
		if (Image == NULL)
			return IL_FALSE;

		if (i == 0) {
			ilTexImage(Image->Width, Image->Height, 1, Image->Bpp, Image->Format, Image->Type, Image->Data);
			Prev = TargetImage;
			Prev->Origin = IL_ORIGIN_UPPER_LEFT;
			ilCloseImage(Image);
		}
		else {
			Prev->Next = Image;
			Prev = Prev->Next;
			iCurImage = Image;
		}
	}

	return ilFixImage();
}


// Internal function to uncompress the .dcx (all .dcx files are rle compressed)
static ILimage *
iUncompressDcx(SIO *io, DCXHEAD *Header) {
	ILubyte		ByteHead, Colour, *ScanLine = NULL /* Only one plane */;
	ILuint		c, i, x, y;//, Read = 0;
	ILimage		*Image = NULL;

	if (Header->Bpp < 8) {
		/*ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;*/
		return iUncompressDcxSmall(io, Header);
	}

	Image = ilNewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;
	/*if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}*/
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	ScanLine = (ILubyte*)ialloc(Header->Bps);
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
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			goto dcx_error;
	}


		/*StartPos = SIO_tell(io);
		Compressed = (ILubyte*)ialloc(Image->SizeOfData * 4 / 3);
		SIOread(io, Compressed, 1, Image->SizeOfData * 4 / 3);

		for (y = 0; y < Image->Height; y++) {
			for (c = 0; c < Image->Bpp; c++) {
				x = 0;
				while (x < Header->Bps) {
					ByteHead = Compressed[Read++];
					if ((ByteHead & 0xC0) == 0xC0) {
						ByteHead &= 0x3F;
						Colour = Compressed[Read++];
						for (i = 0; i < ByteHead; i++) {
							ScanLine[x++] = Colour;
						}
					}
					else {
						ScanLine[x++] = ByteHead;
					}
				}

				for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes ;)
					Image->Data[y * Image->Bps + x * Image->Bpp + c] = ScanLine[x];
				}
			}
		}

		ifree(Compressed);
		SIOseek(io, StartPos + Read, IL_SEEK_SET);*/

	//TODO: because the .pcx-code was broken this
	//code is probably broken, too
	for (y = 0; y < Image->Height; y++) {
		for (c = 0; c < Image->Bpp; c++) {
			x = 0;
			while (x < Header->Bps) {
				if (SIOread(io, &ByteHead, 1, 1) != 1) {
					goto dcx_error;
				}
				if ((ByteHead & 0xC0) == 0xC0) {
					ByteHead &= 0x3F;
					if (SIOread(io, &Colour, 1, 1) != 1) {
						goto dcx_error;
					}
					for (i = 0; i < ByteHead; i++) {
						ScanLine[x++] = Colour;
					}
				}
				else {
					ScanLine[x++] = ByteHead;
				}
			}

			for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes ;)
				Image->Data[y * Image->Bps + x * Image->Bpp + c] = ScanLine[x];
			}
		}
	}

	ifree(ScanLine);

	// Read in the palette
	if (Image->Bpp == 1) {
		ByteHead = SIOgetc(io);	// the value 12, because it signals there's a palette for some reason...
							//	We should do a check to make certain it's 12...
		if (ByteHead != 12)
			SIOseek(io, -1, IL_SEEK_CUR);
		if (SIOread(io, Image->Pal.Palette, 1, Image->Pal.PalSize) != Image->Pal.PalSize) {
			ilCloseImage(Image);
			return NULL;
		}
	}

	return Image;

dcx_error:
	ifree(ScanLine);
	ilCloseImage(Image);
	return NULL;
}


static ILimage *
iUncompressDcxSmall(SIO *io, DCXHEAD *Header) {
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine = NULL;
	ILimage	*Image;

	Image = ilNewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;

	/*if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, 1, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}*/
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
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			ilCloseImage(Image);
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
			ilCloseImage(Image);
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
	ilCloseImage(Image);
	return NULL;
}

ILconst_string iFormatExtsDCX[] = { 
	IL_TEXT("dcx"), 
	NULL 
};

ILformat iFormatDCX = { 
	.Validate = iIsValidDcx, 
	.Load     = iLoadDcxInternal, 
	.Save     = NULL, 
	.Exts     = iFormatExtsDCX
};

#endif//IL_NO_DCX
