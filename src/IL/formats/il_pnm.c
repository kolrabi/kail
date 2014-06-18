//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pnm.c
//
// Description: Reads/writes to/from pbm/pgm/ppm formats (enough slashes? =)
//
//-----------------------------------------------------------------------------



#include "il_internal.h"
#ifndef IL_NO_PNM
#include "il_pnm.h"
#include <limits.h>  // for maximum values
#include <ctype.h>
#include "il_manip.h"

// According to the ppm specs, it's 70, but PSP
//  likes to output longer lines.
#define MAX_BUFFER 180  

static ILboolean	iCheckPnm(char Header[2]);
static ILboolean	ilReadAsciiPpm(ILimage *, PPMINFO *Info);
static ILboolean	ilReadBinaryPpm(ILimage *, PPMINFO *Info);
static ILboolean	ilReadBitPbm(ILimage *, PPMINFO *Info);
static ILboolean	iGetWord(SIO *io, ILboolean);
static void				PbmMaximize(ILimage *Image);

// Global variables FIXME: remove global state
static ILbyte LineBuffer[MAX_BUFFER];
static ILbyte SmallBuff[MAX_BUFFER];
ILstring FName = NULL;

// Can't read direct bits from a lump yet
ILboolean IsLump = IL_FALSE;


// Internal function to get the header and check it.
static ILboolean iIsValidPnm(SIO *io) {
	char	Head[2];
	ILint	Read;

	Read = SIOread(io, Head, 1, 2);
	SIOseek(io, -Read, IL_SEEK_CUR);  // Go ahead and restore to previous state
	if (Read != 2)
		return IL_FALSE;

	return iCheckPnm(Head);
}


// Internal function used to check if the HEADER is a valid .pnm header.
ILboolean iCheckPnm(char Header[2])
{
	if (Header[0] != 'P')
		return IL_FALSE;
	switch (Header[1])
	{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
			return IL_TRUE;
	}

	return IL_FALSE;
}


// Load either a pgm or a ppm
static ILboolean iLoadPnmInternal(ILimage *Image)
{
	PPMINFO		Info;
	ILboolean PmImage = IL_FALSE;
//	ILuint		LineInc = 0, SmallInc = 0;

	Info.Type = 0;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Find out what type of pgm/ppm this is
	if (iGetWord(&Image->io, IL_FALSE) == IL_FALSE)
		return IL_FALSE;

	if (SmallBuff[0] != 'P') {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	switch( SmallBuff[1] ) {
		case '1':
			Info.Type = IL_PBM_ASCII;
			break;
		case '2':
			Info.Type = IL_PGM_ASCII;
			break;
		case '3':
			Info.Type = IL_PPM_ASCII;
			break;
		case '4':
			Info.Type = IL_PBM_BINARY;
			if (IsLump) {
				iSetError(IL_FORMAT_NOT_SUPPORTED);
				return IL_FALSE;
			}
			break;
		case '5':
			Info.Type = IL_PGM_BINARY;
			break;
		case '6':
			Info.Type = IL_PPM_BINARY;
			break;
		default:
			iSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}
	
	// Retrieve the width and height
	if (iGetWord(&Image->io, IL_FALSE) == IL_FALSE)
		return IL_FALSE;
	Info.Width = atoi((const char*)SmallBuff);
	if (Info.Width == 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (iGetWord(&Image->io, IL_FALSE) == IL_FALSE)
		return IL_FALSE;
	Info.Height = atoi((const char*)SmallBuff);
	if (Info.Height == 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Retrieve the maximum colour component value
	if (Info.Type != IL_PBM_ASCII && Info.Type != IL_PBM_BINARY) {
		if (iGetWord(&Image->io, IL_TRUE) == IL_FALSE)
			return IL_FALSE;

		if ((Info.MaxColour = atoi((const char*)SmallBuff)) == 0) {
			iSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}
	} else {
		Info.MaxColour = 1;
	}
	
	if (Info.Type == IL_PBM_ASCII || Info.Type == IL_PBM_BINARY ||
		Info.Type == IL_PGM_ASCII || Info.Type == IL_PGM_BINARY) {
		if (Info.Type == IL_PGM_ASCII) {
			Info.Bpp = Info.MaxColour < 256 ? 1 : 2;
		} else {
			Info.Bpp = 1;
		}
	} else {
		Info.Bpp = 3;
	}
	
	switch (Info.Type) {
		case IL_PBM_ASCII:
		case IL_PGM_ASCII:
		case IL_PPM_ASCII:
			PmImage = ilReadAsciiPpm(Image, &Info);
			break;
		case IL_PBM_BINARY:
			PmImage = ilReadBitPbm(Image, &Info);
			break;
		case IL_PGM_BINARY:
		case IL_PPM_BINARY:
			PmImage = ilReadBinaryPpm(Image, &Info);
			break;
		default:
			return IL_FALSE;
	}

	if (!PmImage) {
    iSetError(IL_FILE_READ_ERROR);
    return IL_FALSE;
	}

	// Is this conversion needed?  Just 0's and 1's shows up as all black
	if (Info.Type == IL_PBM_ASCII) {
		PbmMaximize(Image);
	}

	if (Info.MaxColour > 255)
		Image->Type = IL_UNSIGNED_SHORT;

	Image->Origin = IL_ORIGIN_UPPER_LEFT;
	if ( Info.Type == IL_PBM_ASCII || Info.Type == IL_PBM_BINARY
		|| Info.Type == IL_PGM_ASCII || Info.Type == IL_PGM_BINARY ) {
		Image->Format = IL_LUMINANCE;
	} else {
		Image->Format = IL_RGB;
	}

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}



static ILboolean ilReadAsciiPpm(ILimage *Image, PPMINFO *Info)
{
	ILint	LineInc = 0, SmallInc = 0, DataInc = 0, Size;
//	ILint	BytesRead = 0;

	if (Info->MaxColour > 255)
		Info->Bpp *= 2;

	Size = Info->Width * Info->Height * Info->Bpp;

	if (!iTexImage(Image, Info->Width, Info->Height, 1, (ILubyte)(Info->Bpp), 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_UPPER_LEFT;
	if (Info->MaxColour > 255)
		Image->Type = IL_UNSIGNED_SHORT;

	while (DataInc < Size) {  // && !feof(File)) {
		LineInc = 0;

		if (SIOgets(&Image->io, (char *)LineBuffer, MAX_BUFFER) == NULL) {
			//iSetError(IL_ILLEGAL_FILE_VALUE);
			//return NULL;
			//return Image;
			break;
		}
		if (LineBuffer[0] == '#') {  // Comment
			continue;
		}

		while ((LineBuffer[LineInc] != NUL) && (LineBuffer[LineInc] != '\n')) {

			SmallInc = 0;
			while (!isalnum(LineBuffer[LineInc])) {  // Skip any whitespace
				LineInc++;
			}
			while (isalnum(LineBuffer[LineInc])) {
				SmallBuff[SmallInc] = LineBuffer[LineInc];
				SmallInc++;
				LineInc++;
			}
			SmallBuff[SmallInc] = NUL;
			Image->Data[DataInc] = atoi((const char*)SmallBuff);  // Convert from string to colour

			// PSP likes to put whitespace at the end of lines...figures. =/
			while (!isalnum(LineBuffer[LineInc]) && LineBuffer[LineInc] != NUL) {  // Skip any whitespace
				LineInc++;
			}

			// We should set some kind of state flag that enables this
			//Image->Data[DataInc] *= (ILubyte)(255 / Info->MaxColour);  // Scales to 0-255		
			if (Info->MaxColour > 255)
				DataInc++;
			DataInc++;
		}
	}

	// If we read less than what we should have...
	if (DataInc < Size) {
		//ilCloseImage(Image);
		//ilSetCurImage(NULL);
		iSetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	return IL_TRUE;
}


static ILboolean ilReadBinaryPpm(ILimage *Image, PPMINFO *Info) {
	ILuint	Size;

	Size = Info->Width * Info->Height * Info->Bpp;

	if (!iTexImage(Image, Info->Width, Info->Height, 1, (ILubyte)(Info->Bpp), 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	/* 4/3/2007 Dario Meloni
	 Here it seems we have eaten too much bytes and it is needed to fix 
	 the starting point
	 works well on various images
	
	No more need of this workaround. fixed iGetWord
	Image->io.seek(Image->io.handle, 0,IL_SEEK_END);
	ILuint size = Image->io.tell(Image->io.handle);
	Image->io.seek(Image->io.handle, size-Size,IL_SEEK_SET);
	*/
	if (Image->io.read(Image->io.handle, Image->Data, 1, Size ) != Size) {
		ilCloseImage(Image);	
		return IL_FALSE;
	}
	return IL_TRUE;
}


static ILboolean ilReadBitPbm(ILimage *Image, PPMINFO *Info) {
	ILuint	m, j, x, CurrByte;

	if (!iTexImage(Image, Info->Width, Info->Height, 1, (ILubyte)(Info->Bpp), 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	x = 0;
	for (j = 0; j < Image->SizeOfData;) {
		CurrByte = Image->io.getchar(Image->io.handle);
		for (m = 128; m > 0 && x < Info->Width; m >>= 1, ++x, ++j) {
			Image->Data[j] = (CurrByte & m)?255:0;
		}
		if (x == Info->Width)
			x = 0;
	}

	return IL_TRUE;
}


static ILboolean iGetWord(SIO *io, ILboolean final) {
	ILint WordPos = 0;
	ILint Current = 0;
	ILboolean Started = IL_FALSE;
	ILboolean Looping = IL_TRUE;

	if (SIOeof(io))
		return IL_FALSE;

	while (Looping) {
		while ((Current = SIOgetc(io)) != IL_EOF && Current != '\n' && Current != '#' && Current != ' ') {
			if (WordPos >= MAX_BUFFER)  // We have hit the maximum line length.
				return IL_FALSE;

			if (!isalnum(Current)) {
				if (Started) {
					Looping = IL_FALSE;
					break;
				}
				continue;
			}

			if (Looping)
				SmallBuff[WordPos++] = Current;
		}
		if (Current == IL_EOF)
			return IL_FALSE;
		SmallBuff[WordPos] = 0; // 08-17-2008 - was NULL, changed to avoid warning
		if (final == IL_TRUE)
	        break;

		if (!Looping)
			break;

		if (Current == '#') {  // '#' is a comment...read until end of line
			while ((Current = SIOgetc(io)) != IL_EOF && Current != '\n');
		}

		// Get rid of any erroneous spaces
		while ((Current = SIOgetc(io)) != IL_EOF) {
			if (Current != ' ')
				break;
		}
		SIOseek(io, -1, IL_SEEK_CUR);

		if (WordPos > 0)
			break;
	}

	if (Current == -1 || WordPos == 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	return IL_TRUE;
}

// Internal function used to save the Pnm.
static ILboolean iSavePnmInternal(ILimage *Image)
{
	ILuint		Bpp, MaxVal = UCHAR_MAX, i = 0, j;
	ILenum		Type = 0;
	ILuint		LinePos = 0;  // Cannot exceed 70 for pnm's!
	ILboolean	Binary;
	ILimage		*TempImage;
	ILubyte		*TempData;
	char tmp[512];
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	if (iCheckExtension(FName, IL_TEXT("pbm")))
		Type = IL_PBM_ASCII;
	else if (iCheckExtension(FName, IL_TEXT("pgm")))
		Type = IL_PGM_ASCII;
	else if (iCheckExtension(FName, IL_TEXT("ppm")))
		Type = IL_PPM_ASCII;
	else
		Type = IL_PPM_ASCII;

	/*if (!Type) {
		iSetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}*/

	if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION) {
		Type += 3;
		Binary = IL_TRUE;
	}
	else {
		Binary = IL_FALSE;
	}

	if (Image->Type == IL_UNSIGNED_BYTE) {
		MaxVal = UCHAR_MAX;
	}
	else if (Image->Type == IL_UNSIGNED_SHORT) {
		MaxVal = USHRT_MAX;
	}
	else {
		iSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
	if (MaxVal > UCHAR_MAX && Type >= IL_PBM_BINARY) {  // binary cannot be higher than 255
		iSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	switch (Type)
	{
		case IL_PBM_ASCII:
			Bpp = 1;
			SIOputs(io, "P1\n");
			TempImage = iConvertImage(Image, IL_LUMINANCE, IL_UNSIGNED_BYTE);
			break;
		//case IL_PBM_BINARY:  // Don't want to mess with saving bits just yet...
			//Bpp = 1;
			//ilprintf("P4\n");
			//break;
		case IL_PBM_BINARY:
			iSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
		case IL_PGM_ASCII:
			Bpp = 1;
			SIOputs(io, "P2\n");
			TempImage = iConvertImage(Image, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			break;
		case IL_PGM_BINARY:
			Bpp = 1;
			SIOputs(io, "P5\n");
			TempImage = iConvertImage(Image, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			break;
		case IL_PPM_ASCII:
			Bpp = 3;
			SIOputs(io, "P3\n");
			TempImage = iConvertImage(Image, IL_RGB, IL_UNSIGNED_BYTE);
			break;
		case IL_PPM_BINARY:
			Bpp = 3;
			SIOputs(io, "P6\n");
			TempImage = iConvertImage(Image, IL_RGB, IL_UNSIGNED_BYTE);
			break;
		default:
			iSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	if (TempImage == NULL)
		return IL_FALSE;

	if (Bpp != TempImage->Bpp) {
		iSetError(IL_INVALID_CONVERSION);
		return IL_FALSE;
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	snprintf(tmp, sizeof(tmp), "%d %d\n", TempImage->Width, TempImage->Height);
	SIOputs(io, tmp);

	if (Type != IL_PBM_BINARY && Type != IL_PBM_ASCII) { // not needed for .pbm's (only 0 and 1) 
		snprintf(tmp, sizeof(tmp), "%d\n", MaxVal);
		SIOputs(io, tmp);
	}

	while (i < TempImage->SizeOfPlane) {
		for (j = 0; j < Bpp; j++) {
			if (Binary) {
				if (Type == IL_PBM_BINARY) {
					Image->io.putchar((ILubyte)(TempData[i] > 127 ? 1 : 0), Image->io.handle);
				}
				else {
					Image->io.putchar(TempData[i], Image->io.handle);
				}
			}
			else {
				/*
				if (TempImage->Type == IL_UNSIGNED_BYTE)
					k = TempData[i];
				else  // IL_UNSIGNED_SHORT
					k = *((ILushort*)TempData + i);
				*/
				if (Type == IL_PBM_ASCII) {
					if (TempData[i] > 127)
						SIOputs(io, "1 ");
					else
						SIOputs(io, "0 ");
					LinePos += 2;
				}
				else {
					char tmp[32];
					LinePos += snprintf(tmp, sizeof(tmp), "%d ", TempData[i]);
					SIOputs(io, tmp);
				}
			}

			if (TempImage->Type == IL_UNSIGNED_SHORT)
				i++;
			i++;
		}

		if (LinePos > 65) {  // Just a good number =]
			SIOputs(io, "\n");
			LinePos = 0;
		}
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT)
		ifree(TempData);
	ilCloseImage(TempImage);

	return IL_TRUE;
}


// Converts a .pbm to something viewable.
void PbmMaximize(ILimage *Image)
{
	ILuint i = 0;
	for (i = 0; i < Image->SizeOfPlane; i++)
		if (Image->Data[i] == 1)
			Image->Data[i] = 0xFF;
	return;
}

ILconst_string iFormatExtsPNM[] = { 
  IL_TEXT("pbm"), 
  IL_TEXT("pgm"), 
  IL_TEXT("pnm"), 
  IL_TEXT("ppm"), 
  NULL 
};

ILformat iFormatPNM = { 
  /* .Validate = */ iIsValidPnm, 
  /* .Load     = */ iLoadPnmInternal, 
  /* .Save     = */ iSavePnmInternal, 
  /* .Exts     = */ iFormatExtsPNM
};

#endif//IL_NO_PNM
