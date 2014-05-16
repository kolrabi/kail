//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 10/10/2006
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
#include "il_bits.h"


// Wrap it just in case...
#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif
typedef struct CUT_HEAD
{
	ILushort	Width;
	ILushort	Height;
	ILushort	Dummy;
} IL_PACKSTRUCT CUT_HEAD;
#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

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


ILboolean isValidCutHeader(const CUT_HEAD* header)
{
	if (header == 0)
		return false;

	if (header->Width > 0
	&&  header->Height > 0
	&&  header->Dummy == 0) 
	{
		return true;
	} else {
		return false;
	}
}

// Simple check if the file header is plausible
ILboolean iIsValidCut(SIO* io)
{
	if (io == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	CUT_HEAD	header;
	io->read(io->handle, &header, 1, sizeof(CUT_HEAD));
	return isValidCutHeader(&header);
}

bool readScanLine(ILimage* image, ILubyte* chunk, ILushort chunkSize, int y)
{
	ILushort chunkOffset = 0;
	ILuint outOffset = 0;
	ILubyte* data = &image->Data[image->Bps * y];
	bool errorOccurred = false;

	while (chunkOffset < chunkSize && outOffset < image->Width) {
		ILuint controlByte = chunk[chunkOffset];
		if (controlByte > 128) {
			if (chunkOffset+1 < chunkSize) {
				// RLE decoding
				ILubyte value = chunk[chunkOffset+1];
				auto toCopy = min(controlByte-128, image->Width-outOffset);
				memset(&data[outOffset], value, toCopy);
				chunkOffset += 2;
				outOffset += toCopy;
			} else {
				chunkOffset = chunkSize; // done
				errorOccurred = true;
			}
		} else {
			ILuint bytesToCopy = controlByte;
			if (chunkOffset+bytesToCopy+1 < chunkSize) {
				// Raw copying
				bytesToCopy = min(bytesToCopy, image->Width-outOffset);
				memcpy(&data[outOffset], &chunk[chunkOffset+1], bytesToCopy);
				chunkOffset += bytesToCopy+1;
				outOffset += bytesToCopy;
			} else {
				chunkOffset = chunkSize; // done
				errorOccurred = true;
			}
		}
	}

	return errorOccurred;
}

ILboolean iLoadCutInternal(ILimage* image)
{
	if (image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	CUT_HEAD	Header;
	image->io.read(image->io.handle, &Header, 1, sizeof(Header));
	if (Header.Width == 0 || Header.Height == 0) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {  // always 1 bpp
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	ILubyte* chunk = (ILubyte*) ialloc(64 * 1024); // max. size of a data chunk
	bool done = false;
	int y = 0;

	while (!done) {
		ILushort chunkSize;
		auto read = image->io.read(image->io.handle, &chunkSize, 1, sizeof(chunkSize));
		if (read == sizeof(chunkSize)) {
			if (image->io.read(image->io.handle, chunk, 1, chunkSize) == chunkSize) {
				done = readScanLine(image, chunk, chunkSize, y);
				++y;
			} else 
				done = true;
		} else 
			done = true;
	}

	ifree(chunk);
	image->Origin = IL_ORIGIN_UPPER_LEFT;  // Not sure

	image->Pal.PalSize = 256 * 3; // never larger than 768
	image->Pal.PalType = IL_PAL_RGB24;
	image->Pal.Palette = (ILubyte*)ialloc(image->Pal.PalSize);

	// Create a fake greyscale palette
	for (int i = 0; i < 256; ++i) {
		image->Pal.Palette[3*i  ] = i;
		image->Pal.Palette[3*i+1] = i;
		image->Pal.Palette[3*i+2] = i;
	}

	return ilFixImage();
}


#endif//IL_NO_CUT
