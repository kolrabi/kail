//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_sgi.c
//
// Description: Reads and writes Silicon Graphics Inc. (.sgi) files.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_SGI

#include "il_sgi.h"
#include "il_manip.h"
#include "algo/il_rle.h"

#include <limits.h>

// static char *FName = NULL;

// Internal functions
static ILboolean	iCheckSgi(iSgiHeader *Header);
//void		iExpandScanLine(ILubyte *Dest, ILubyte *Src, ILuint Bpc);
static ILuint			iGetScanLine(SIO *, ILubyte *ScanLine, iSgiHeader *Head, ILuint Length);
static ILboolean	iNewSgi(ILimage *, iSgiHeader *Head);
static ILboolean	iReadNonRleSgi(ILimage *, iSgiHeader *Head);
static ILboolean	iReadRleSgi(ILimage *, iSgiHeader *Head);
static ILboolean 	iSaveRleSgi(SIO *, 	ILubyte *Data, ILuint w, ILuint h, ILuint numChannels, ILuint bps);

#ifdef WORDS_LITTLEENDIAN
static void				sgiSwitchData(ILubyte *Data, ILuint SizeOfData);
#endif

/*----------------------------------------------------------------------------*/

// Internal function used to get the .sgi header from the current file.
static ILuint iGetSgiHead(SIO *io, iSgiHeader *Header)
{
	ILuint read = SIOread(io, Header, 1, sizeof(iSgiHeader));

	BigShort (&Header->MagicNum);
	BigUShort(&Header->Dim);
	BigUShort(&Header->XSize);
	BigUShort(&Header->YSize);
	BigUShort(&Header->ZSize);
	BigInt   (&Header->PixMin);
	BigInt   (&Header->PixMax);
	BigInt   (&Header->Dummy1);
	BigInt   (&Header->ColMap);

	return read;
}

/*----------------------------------------------------------------------------*/

/* Internal function used to check if the HEADER is a valid .sgi header. */
static ILboolean iCheckSgi(iSgiHeader *Header)
{
	if (Header->MagicNum != SGI_MAGICNUM)
		return IL_FALSE;
	if (Header->Storage != SGI_RLE && Header->Storage != SGI_VERBATIM)
		return IL_FALSE;
	if (Header->Bpc == 0 || Header->Dim == 0)
		return IL_FALSE;
	if (Header->XSize == 0 || Header->YSize == 0 || Header->ZSize == 0)
		return IL_FALSE;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/* Internal function to get the header and check it. */
static ILboolean iIsValidSgi(SIO *io)
{
	iSgiHeader	Head;
	ILuint      Start = SIOtell(io);
	ILuint 			read = iGetSgiHead(io, &Head);
	
	SIOseek(io, Start, IL_SEEK_SET);  // Restore previous file position

	return read == sizeof(Head) && iCheckSgi(&Head);
}

/*----------------------------------------------------------------------------*/

static ILboolean iReadRleSgi(ILimage *Image, iSgiHeader *Head)
{
	ILuint 		ixTable;
	ILuint 		ChanInt = 0;
	ILuint  	ixPlane, ixHeight,ixPixel, RleOff, RleLen;
	ILuint		*OffTable=NULL, *LenTable=NULL, TableSize, Cur;
	ILubyte		**TempData=NULL;
	SIO *			io = &Image->io;

	if (!iNewSgi(Image, Head))
		return IL_FALSE;

	TableSize = Head->YSize * Head->ZSize;
	OffTable  = (ILuint*)ialloc(TableSize * sizeof(ILuint));
	LenTable  = (ILuint*)ialloc(TableSize * sizeof(ILuint));

	if (OffTable == NULL || LenTable == NULL)
		goto cleanup_error;

	if (SIOread(io, OffTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;

	if (SIOread(io, LenTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;

	// Fix the offset/len table (it's big endian format)
	for (ixTable = 0; ixTable < TableSize; ixTable++) {
		BigUInt(OffTable + ixTable);
		BigUInt(LenTable + ixTable);
	}

	// We have to create a temporary buffer for the image, because SGI
	//	images are plane-separated.
	TempData = (ILubyte**)ialloc(Head->ZSize * sizeof(ILubyte*));
	if (TempData == NULL)
		goto cleanup_error;

	imemclear(TempData, Head->ZSize * sizeof(ILubyte*));  // Just in case ialloc fails then cleanup_error.
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		TempData[ixPlane] = (ILubyte*)ialloc(Head->XSize * Head->YSize * Head->Bpc);
		if (TempData[ixPlane] == NULL)
			goto cleanup_error;
	}

	// Read the Planes into the temporary memory
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		for (ixHeight = 0, Cur = 0;	ixHeight < Head->YSize;
			ixHeight++, Cur += Head->XSize * Head->Bpc) {
      ILuint Scan;

			RleOff = OffTable[ixHeight + ixPlane * Head->YSize];
			RleLen = LenTable[ixHeight + ixPlane * Head->YSize];
			
			// Seeks to the offset table position
			SIOseek(io, RleOff, IL_SEEK_SET);
			Scan = iGetScanLine(io, (TempData[ixPlane]) + (ixHeight * Head->XSize * Head->Bpc),	Head, RleLen);
			if (Scan != Head->XSize * Head->Bpc) {
					iSetError(IL_ILLEGAL_FILE_VALUE);
					goto cleanup_error;
			}
		}
	}

	// DW: Removed on 05/25/2002.
	/*// Check if an alphaplane exists and invert it
	if (Head->ZSize == 4) {
		for (ixPixel=0; (ILint)ixPixel<Head->XSize * Head->YSize; ixPixel++) {
 			TempData[3][ixPixel] = TempData[3][ixPixel] ^ 255;
 		}	
	}*/
	
	// Assemble the image from its planes
	for (ixPixel = 0; ixPixel < Image->SizeOfData;
		ixPixel += Head->ZSize * Head->Bpc, ChanInt += Head->Bpc) {
		for (ixPlane = 0; (ILint)ixPlane < Head->ZSize * Head->Bpc;	ixPlane += Head->Bpc) {
			Image->Data[ixPixel + ixPlane] = TempData[ixPlane][ChanInt];
			if (Head->Bpc == 2)
				Image->Data[ixPixel + ixPlane + 1] = TempData[ixPlane][ChanInt + 1];
		}
	}

	#ifdef WORDS_LITTLEENDIAN
	if (Head->Bpc == 2)
		sgiSwitchData(Image->Data, Image->SizeOfData);
	#endif

	ifree(OffTable);
	ifree(LenTable);

	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		ifree(TempData[ixPlane]);
	}
	ifree(TempData);

	return IL_TRUE;

cleanup_error:
	ifree(OffTable);
	ifree(LenTable);
	if (TempData) {
		for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
			ifree(TempData[ixPlane]);
		}
		ifree(TempData);
	}

	return IL_FALSE;
}

/*----------------------------------------------------------------------------*/

static ILuint iGetScanLine(SIO *io, ILubyte *ScanLine, iSgiHeader *Head, ILuint Length) {
	ILushort Pixel, Count;  // For current pixel
	ILuint	 BppRead = 0, CurPos = 0, Bps = Head->XSize * Head->Bpc;

	while (BppRead < Length && CurPos < Bps)
	{
		Pixel = 0;
		if (SIOread(io, &Pixel, Head->Bpc, 1) != 1)
			return ~0UL;
		
		UShort(&Pixel);

		Count = Pixel & 0x7f;
		if (!Count)  // If 0, line ends
			return CurPos;

		if (Pixel & 0x80) {  // If top bit set, then it is a "run"
			if (SIOread(io, ScanLine, Head->Bpc, Count) != Count)
				return ~0UL;

			BppRead += Head->Bpc * Count + Head->Bpc;
			ScanLine += Head->Bpc * Count;
			CurPos += Head->Bpc * Count;
		}	else {
			if (SIOread(io, &Pixel, Head->Bpc, 1) != 1)
				return ~0UL;

			UShort(&Pixel);
			if (Head->Bpc == 1) {
				while (Count--) {
					*ScanLine = (ILubyte)Pixel;
					ScanLine++;
					CurPos++;
				}
			}
			else {
				while (Count--) {
					*(ILushort*)(void*)ScanLine = Pixel;
					ScanLine += 2;
					CurPos += 2;
				}
			}
			BppRead += Head->Bpc + Head->Bpc;
		}
	}
	return CurPos;
}


/*----------------------------------------------------------------------------*/

// Much easier to read - just assemble from planes, no decompression
static ILboolean iReadNonRleSgi(ILimage *Image, iSgiHeader *Head)
{
	ILuint		i, c;
	// ILboolean	Cache = IL_FALSE;
	SIO *     io 		= &Image->io;

	if (!iNewSgi(Image, Head)) {
		return IL_FALSE;
	}

	for (c = 0; c < Image->Bpp; c++) {
		for (i = c; i < Image->SizeOfData; i += Image->Bpp) {
			if (SIOread(io, Image->Data + i, 1, 1) != 1) {
				return IL_FALSE;
			}
		}
	}

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

#ifdef WORDS_LITTLEENDIAN

static void sgiSwitchData(ILubyte *Data, ILuint SizeOfData) {	
	ILubyte	Temp;
	ILuint	i;
	for (i = 0; i < SizeOfData; i += 2) {
		Temp = Data[i];
		Data[i] = Data[i+1];
		Data[i+1] = Temp;
	}
	return;
}

#endif

/*----------------------------------------------------------------------------*/

// Just an internal convenience function for reading SGI files
static ILboolean iNewSgi(ILimage *Image, iSgiHeader *Head)
{
	if (!iTexImage(Image, Head->XSize, Head->YSize, Head->Bpc, (ILubyte)Head->ZSize, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_LOWER_LEFT;

	switch (Head->ZSize)
	{
		case 1:
			Image->Format = IL_LUMINANCE;
			break;
		/*case 2:
			Image->Format = IL_LUMINANCE_ALPHA; 
			break;*/
		case 3:
			Image->Format = IL_RGB;
			break;
		case 4:
			Image->Format = IL_RGBA;
			break;
		default:
			iSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	switch (Head->Bpc)
	{
		case 1:
			if (Head->PixMin < 0)
				Image->Type = IL_BYTE;
			else
				Image->Type = IL_UNSIGNED_BYTE;
			break;
		case 2:
			if (Head->PixMin < 0)
				Image->Type = IL_SHORT;
			else
				Image->Type = IL_UNSIGNED_SHORT;
			break;
		default:
			iSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	Image->Origin = IL_ORIGIN_LOWER_LEFT;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

static ILenum DetermineSgiType(ILenum Type)
{
	if (Type == IL_INT)
		return IL_SHORT;

	if (Type > IL_UNSIGNED_SHORT) {
		return IL_UNSIGNED_SHORT;
	}

	return Type;
}

/*----------------------------------------------------------------------------*/

// Rle does NOT work yet.

// Internal function used to save the Sgi.
// @todo: write header in one call
static ILboolean iSaveSgiInternal(ILimage *Image)
{
	ILuint		i, c;
	ILboolean	Compress;
	ILimage		*Temp = Image;
	ILubyte		*TempData;
	SIO * 		io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	if (Image->Format != IL_LUMINANCE
	    //while the sgi spec doesn't directly forbid rgb files with 2
	    //channels, they are quite uncommon and most apps don't support
	    //them. so convert lum_a images to rgba before writing.
	    //&& Image->Format != IL_LUMINANCE_ALPHA
	    && Image->Format != IL_RGB
	    && Image->Format != IL_RGBA) {
		if (Image->Format == IL_BGRA || Image->Format == IL_LUMINANCE_ALPHA)
			Temp = iConvertImage(Image, IL_RGBA, DetermineSgiType(Image->Type));
		else
			Temp = iConvertImage(Image, IL_RGB, DetermineSgiType(Image->Type));
	}
	else if (Image->Type > IL_UNSIGNED_SHORT) {
		Temp = iConvertImage(Image, Image->Format, DetermineSgiType(Image->Type));
	}
	
	if (Temp == NULL)
		return IL_FALSE;

	//compression of images with 2 bytes per channel doesn't work yet
	Compress = ilIsEnabled(IL_SGI_RLE) && Temp->Bpc == 1;

	SaveBigUShort(io, SGI_MAGICNUM);  // 'Magic' number
	if (Compress)
		SIOputc(io, 1);
	else
		SIOputc(io, 0);

	if (Temp->Type == IL_UNSIGNED_BYTE)
		SIOputc(io, 1);
	else if (Temp->Type == IL_UNSIGNED_SHORT)
		SIOputc(io, 2);
	// Need to error here if not one of the two...

	if (Temp->Format == IL_LUMINANCE || Temp->Format == IL_COLOUR_INDEX)
		SaveBigUShort(io, 2);
	else
		SaveBigUShort(io, 3);

	SaveBigUShort(io, (ILushort)Temp->Width);
	SaveBigUShort(io, (ILushort)Temp->Height);
	SaveBigUShort(io, (ILushort)Temp->Bpp);

	switch (Temp->Type)
	{
		case IL_BYTE:
			SaveBigInt(io, SCHAR_MIN);	// Minimum pixel value
			SaveBigInt(io, SCHAR_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_BYTE:
			SaveBigInt(io, 0);			// Minimum pixel value
			SaveBigInt(io, UCHAR_MAX);	// Maximum pixel value
			break;
		case IL_SHORT:
			SaveBigInt(io, SHRT_MIN);	// Minimum pixel value
			SaveBigInt(io, SHRT_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_SHORT:
			SaveBigInt(io, 0);			// Minimum pixel value
			SaveBigInt(io, USHRT_MAX);	// Maximum pixel value
			break;
	}

	SaveBigInt(io, 0);  // Dummy value

	/*if (FName) {
		c = ilCharStrLen(FName);
		c = c < 79 ? 79 : c;
		SIOwrite(io, FName, 1, c);
		c = 80 - c;
		for (i = 0; i < c; i++) {
			SIOputc(io, 0);
		}
	}
	else {
		*/
		for (i = 0; i < 80; i++) {
			SIOputc(io, 0);
		}
	//}

	SaveBigUInt(io, 0);  // Colormap

	// Padding
	for (i = 0; i < 101; i++) {
		SaveLittleInt(io, 0);
	}


	if (Image->Origin == IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(Temp);
		if (TempData == NULL) {
			if (Temp!= Image)
				iCloseImage(Temp);
			return IL_FALSE;
		}
	}
	else {
		TempData = Temp->Data;
	}


	if (!Compress) {
		for (c = 0; c < Temp->Bpp; c++) {
			for (i = c; i < Temp->SizeOfData; i += Temp->Bpp) {
				SIOputc(io, TempData[i]);  // Have to save each colour plane separately.
			}
		}
	}
	else {
		iSaveRleSgi(io, TempData, Temp->Width, Temp->Height, Temp->Bpp, Temp->Bps);
	}


	if (TempData != Temp->Data)
		ifree(TempData);
	if (Temp != Image)
		iCloseImage(Temp);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

static ILboolean iSaveRleSgi(SIO *io, ILubyte *Data, ILuint w, ILuint h, ILuint numChannels,
		ILuint bps)
{
	//works only for sgi files with only 1 bpc

	ILuint	c, i, y, j;
	ILubyte	*ScanLine = NULL, *CompLine = NULL;
	ILuint	*StartTable = NULL, *LenTable = NULL;
	ILuint	TableOff, DataOff = 0;

	ScanLine = (ILubyte*)ialloc(w);
	CompLine = (ILubyte*)ialloc(w * 2 + 1);  // Absolute worst case.
	StartTable = (ILuint*)ialloc(h * numChannels * sizeof(ILuint));
	LenTable = (ILuint*)icalloc(h * numChannels, sizeof(ILuint));
	if (!ScanLine || !CompLine || !StartTable || !LenTable) {
		ifree(ScanLine);
		ifree(CompLine);
		ifree(StartTable);
		ifree(LenTable);
		return IL_FALSE;
	}

	// These just contain dummy values at this point.
	TableOff = SIOtell(io);
	SIOwrite(io, StartTable, sizeof(ILuint), h * numChannels);
	SIOwrite(io, LenTable,   sizeof(ILuint), h * numChannels);

	DataOff = SIOtell(io);
	for (c = 0; c < numChannels; c++) {
		for (y = 0; y < h; y++) {
			i = y * bps + c;
			for (j = 0; j < w; j++, i += numChannels) {
				ScanLine[j] = Data[i];
			}

			iRleCompressLine(ScanLine, w, 1, CompLine, LenTable + h * c + y, IL_SGICOMP);
			SIOwrite(io, CompLine, 1, *(LenTable + h * c + y));
		}
	}

	SIOseek(io, TableOff, IL_SEEK_SET);

	j = h * numChannels;
	for (y = 0; y < j; y++) {
		StartTable[y] = DataOff;
		DataOff += LenTable[y];
		BigUInt(&StartTable[y]);
 		BigUInt(&LenTable[y]);
	}

	SIOwrite(io, StartTable, sizeof(ILuint), h * numChannels);
	SIOwrite(io, LenTable,   sizeof(ILuint), h * numChannels);

	ifree(ScanLine);
	ifree(CompLine);
	ifree(StartTable);
	ifree(LenTable);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/* Internal function used to load the SGI image */
static ILboolean iLoadSgiInternal(ILimage *Image)
{
	iSgiHeader	Header;
	ILboolean		bSgi;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetSgiHead(&Image->io, &Header))
		return IL_FALSE;

	if (!iCheckSgi(&Header)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Bugfix for #1060946.
	//  The ZSize should never really be 2 by the specifications.  Some
	//  application is outputting these, and it looks like the ZSize
	//  should really be 1.
	if (Header.ZSize == 2)
		Header.ZSize = 1;
	
	if (Header.Storage == SGI_RLE) {  // RLE
		bSgi = iReadRleSgi(Image, &Header);
	}
	else {  // Non-RLE  //(Header.Storage == SGI_VERBATIM)
		bSgi = iReadNonRleSgi(Image, &Header);
	}

	if (!bSgi)
		return IL_FALSE;
	return IL_TRUE;
}

static ILconst_string iFormatExtsSGI[] = { 
	IL_TEXT("sgi"), 
	IL_TEXT("bw"), 
	IL_TEXT("rgb"), 
	IL_TEXT("rgba"), 
	NULL 
};

ILformat iFormatSGI = { 
	/* .Validate = */ iIsValidSgi, 
	/* .Load     = */ iLoadSgiInternal, 
	/* .Save     = */ iSaveSgiInternal, 
	/* .Exts     = */ iFormatExtsSGI
};

#endif//IL_NO_SGI
