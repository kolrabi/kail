//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 2014-05-21 by Björn Paetzel
//
// Filename: src-IL/src/il_cut.c
//
// Description: Reads a Dr. Halo .cut file
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_CUT
#include "il_manip.h"
#include "il_pal.h"

// Wrap it just in case...
#include "pack_push.h"
typedef struct CUT_HEAD
{
	ILushort	Width;
	ILushort	Height;
	ILushort	Dummy;
} CUT_HEAD;
#include "pack_pop.h"

#if 0
struct HaloPalette
{
	ILubyte  FileId[2];          /* 00h   File Identifier - always "AH" */
	ILushort  Version;            /* 02h   File Version */
	ILushort  Size;               /* 04h   File Size in Bytes minus header */
	ILubyte  FileType;           /* 06h   Palette File Identifier   */
	ILubyte  SubType;            /* 07h   Palette File Subtype   */
	ILushort  BoardId;            /* 08h   Board ID Code */
	ILushort  GraphicsMode;       /* 0Ah   Graphics Mode of Stored Image   */
	ILushort  MaxIndex;           /* 0Ch   Maximum Color Palette Index   */
	ILushort  MaxRed;             /* 0Eh   Maximum Red Palette Value   */
	ILushort  MaxGreen;           /* 10h   Maximum Green Palette Value   */
	ILushort  MaxBlue;            /* 12h   Maximum Blue Color Value   */
	ILubyte  PaletteId[20];      /* 14h   Identifier String "Dr. Halo" */
};
#endif

ILboolean isValidCutHeader(const CUT_HEAD* header)
{
	if (header == 0)
		return IL_FALSE;

	if (header->Width > 0
	&&  header->Height > 0
	&&  header->Dummy == 0) 
	{
		return IL_TRUE;
	} else {
		return IL_FALSE;
	}
}

// Simple check if the file header is plausible
static ILboolean 
iIsValidCut(SIO* io) {
	CUT_HEAD	header;

	if (io == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	return SIOread(io, &header, 1, sizeof(CUT_HEAD)) == sizeof(CUT_HEAD) && isValidCutHeader(&header);
}

ILboolean readScanLine(ILimage* image, ILubyte* chunk, ILushort chunkSize, int y)
{
	ILushort chunkOffset = 0;
	ILuint outOffset = 0;
	ILubyte* data = &image->Data[image->Bps * y];
	ILboolean errorOccurred = IL_FALSE;

	while (chunkOffset < chunkSize && outOffset < image->Width) {
		ILuint controlByte = chunk[chunkOffset];
		if (controlByte > 128) {
			if (chunkOffset+1 < chunkSize) {
				// RLE decoding
				ILubyte value = chunk[chunkOffset+1];
				ILuint toCopy = IL_MIN(controlByte-128, image->Width-outOffset);
				memset(&data[outOffset], value, toCopy);
				chunkOffset += 2;
				outOffset += toCopy;
			} else {
				chunkOffset = chunkSize; // done
				errorOccurred = IL_TRUE;
			}
		} else {
			ILuint bytesToCopy = controlByte;
			if (chunkOffset+bytesToCopy+1 < chunkSize) {
				// Raw copying
				bytesToCopy = IL_MIN(bytesToCopy, image->Width-outOffset);
				memcpy(&data[outOffset], &chunk[chunkOffset+1], bytesToCopy);
				chunkOffset += bytesToCopy+1;
				outOffset += bytesToCopy;
			} else {
				chunkOffset = chunkSize; // done
				errorOccurred = IL_TRUE;
			}
		}
	}

	return errorOccurred;
}

static ILboolean 
iLoadCutInternal(ILimage* image) {
	SIO *       io;
	CUT_HEAD	Header;
    ILubyte *   chunk;
	ILboolean   done    = IL_FALSE;
	ILint       y       = 0, i;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;

	if ( SIOread(io, &Header, 1, sizeof(Header)) != sizeof(Header) 
	  || Header.Width == 0 
	  || Header.Height == 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!iTexImage(image, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {  // always 1 bpp
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	chunk = (ILubyte*) ialloc(64 * 1024); // max. size of a data chunk

	while (!done) {
		ILushort chunkSize;
		ILuint read = SIOread(&image->io, &chunkSize, 1, sizeof(chunkSize));
		if (read == sizeof(chunkSize)) {
			if (SIOread(&image->io, chunk, 1, chunkSize) == chunkSize) {
				done = readScanLine(image, chunk, chunkSize, y);
				++y;
			} else 
				done = IL_TRUE;
		} else 
			done = IL_TRUE;
	}

	ifree(chunk);
	image->Origin = IL_ORIGIN_UPPER_LEFT;  // Not sure

	image->Pal.PalSize = 256 * 3; // never larger than 768
	image->Pal.PalType = IL_PAL_RGB24;
	image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);

	// Create a fake greyscale palette
	for (i = 0; i < 256; ++i) {
		image->Pal.Palette[3*i  ] = i;
		image->Pal.Palette[3*i+1] = i;
		image->Pal.Palette[3*i+2] = i;
	}

	return IL_TRUE;
}

ILconst_string iFormatExtsCUT[] = { 
	IL_TEXT("cut"), 
	NULL 
};

ILformat iFormatCUT = { 
	/* .Validate = */ iIsValidCut, 
	/* .Load     = */ iLoadCutInternal, 
	/* .Save     = */ NULL, 
	/* .Exts     = */ iFormatExtsCUT
};

#endif//IL_NO_CUT
