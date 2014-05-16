//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_psp.c
//
// Description: Reads a Paint Shop Pro file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_psp.h"
#ifndef IL_NO_PSP

// Global variables
ILubyte PSPSignature[32] = {
	0x50, 0x61, 0x69, 0x6E, 0x74, 0x20, 0x53, 0x68, 0x6F, 0x70, 0x20, 0x50, 0x72, 0x6F, 0x20, 0x49,
	0x6D, 0x61, 0x67, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x0A, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00
};

ILubyte GenAttHead[4] = {
	0x7E, 0x42, 0x4B, 0x00
};


// Make these global, since they contain most of the image information.
GENATT_CHUNK	AttChunk;
PSPHEAD			Header;
ILuint			NumChannels;
ILubyte			**Channels = NULL;
ILubyte			*Alpha = NULL;
ILpal			Pal;


// Internal function used to get the Psp header from the current file.
ILboolean iGetPspHead()
{
	if (iCurImage->io.read(iCurImage->io.handle, Header.FileSig, 1, 32) != 32)
		return IL_FALSE;
	Header.MajorVersion = GetLittleUShort(&iCurImage->io);
	Header.MinorVersion = GetLittleUShort(&iCurImage->io);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPsp()
{
	if (!iGetPspHead())
		return IL_FALSE;
	iCurImage->io.seek(iCurImage->io.handle, -(ILint)sizeof(PSPHEAD), IL_SEEK_CUR);

	return iCheckPsp();
}


// Internal function used to check if the HEADER is a valid Psp header.
ILboolean iCheckPsp()
{
	if (_stricmp(Header.FileSig, "Paint Shop Pro Image File\n\x1a"))
		return IL_FALSE;
	if (Header.MajorVersion < 3 || Header.MajorVersion > 5)
		return IL_FALSE;
	if (Header.MinorVersion != 0)
		return IL_FALSE;


	return IL_TRUE;
}


//! Reads a PSP file
/*ILboolean ilLoadPsp(ILconst_string FileName)
{
	ILHANDLE	PSPFile;
	ILboolean	bPsp = IL_FALSE;

	PSPFile = iCurImage->io.openReadOnly(FileName);
	if (PSPFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPsp;
	}

	bPsp = ilLoadPspF(PSPFile);
	iCurImage->io.close(PSPFile);

	return bPsp;
}


//! Reads an already-opened PSP file
ILboolean ilLoadPspF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = iCurImage->io.tell(iCurImage->io.handle);
	bRet = iLoadPspInternal();
	iCurImage->io.seek(iCurImage->io.handle, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a PSP
ILboolean ilLoadPspL(const void *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadPspInternal();
}*/


// Internal function used to load the PSP.
ILboolean iLoadPspInternal()
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Channels = NULL;
	Alpha = NULL;
	Pal.Palette = NULL;

	if (!iGetPspHead())
		return IL_FALSE;
	if (!iCheckPsp()) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadGenAttributes())
		return IL_FALSE;
	if (!ParseChunks())
		return IL_FALSE;
	if (!AssembleImage())
		return IL_FALSE;

	Cleanup();
	return ilFixImage();
}


ILboolean ReadGenAttributes()
{
	BLOCKHEAD		AttHead;
	ILint			Padding;
	ILuint			ChunkLen;

	if (iCurImage->io.read(iCurImage->io.handle, &AttHead, sizeof(AttHead), 1) != 1)
		return IL_FALSE;
	UShort(&AttHead.BlockID);
	UInt(&AttHead.BlockLen);

	if (AttHead.HeadID[0] != 0x7E || AttHead.HeadID[1] != 0x42 ||
		AttHead.HeadID[2] != 0x4B || AttHead.HeadID[3] != 0x00) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	if (AttHead.BlockID != PSP_IMAGE_BLOCK) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	ChunkLen = GetLittleUInt(&iCurImage->io);
	if (Header.MajorVersion != 3)
		ChunkLen -= 4;
	if (iCurImage->io.read(iCurImage->io.handle, &AttChunk, IL_MIN(sizeof(AttChunk), ChunkLen), 1) != 1)
		return IL_FALSE;

	// Can have new entries in newer versions of the spec (4.0).
	Padding = (ChunkLen) - sizeof(AttChunk);
	if (Padding > 0)
		iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);

	// @TODO:  Anything but 24 not supported yet...
	if (AttChunk.BitDepth != 24 && AttChunk.BitDepth != 8) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO;  Add support for compression...
	if (AttChunk.Compression != PSP_COMP_NONE && AttChunk.Compression != PSP_COMP_RLE) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO: Check more things in the general attributes chunk here.

	return IL_TRUE;
}


ILboolean ParseChunks()
{
	BLOCKHEAD	Block;
	ILuint		Pos;

	do {
		if (iCurImage->io.read(iCurImage->io.handle, &Block, 1, sizeof(Block)) != sizeof(Block)) {
			ilGetError();  // Get rid of the erroneous IL_FILE_READ_ERROR.
			return IL_TRUE;
		}
		if (Header.MajorVersion == 3)
			Block.BlockLen = GetLittleUInt(&iCurImage->io);
		else
			UInt(&Block.BlockLen);

		if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
			Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
				return IL_TRUE;
		}
		UShort(&Block.BlockID);
		UInt(&Block.BlockLen);

		Pos = iCurImage->io.tell(iCurImage->io.handle);

		switch (Block.BlockID)
		{
			case PSP_LAYER_START_BLOCK:
				if (!ReadLayerBlock(Block.BlockLen))
					return IL_FALSE;
				break;

			case PSP_ALPHA_BANK_BLOCK:
				if (!ReadAlphaBlock(Block.BlockLen))
					return IL_FALSE;
				break;

			case PSP_COLOR_BLOCK:
				if (!ReadPalette(Block.BlockLen))
					return IL_FALSE;
				break;

			// Gets done in the next iseek, so this is now commented out.
			//default:
				//iCurImage->io.seek(iCurImage->io.handle, Block.BlockLen, IL_SEEK_CUR);
		}

		// Skip to next block just in case we didn't read the entire block.
		iCurImage->io.seek(iCurImage->io.handle, Pos + Block.BlockLen, IL_SEEK_SET);

		// @TODO: Do stuff here.

	} while (1);

	return IL_TRUE;
}


ILboolean ReadLayerBlock(ILuint BlockLen)
{
	BLOCKHEAD			Block;
	LAYERINFO_CHUNK		LayerInfo;
	LAYERBITMAP_CHUNK	Bitmap;
	ILuint				ChunkSize, Padding, i, j;
	ILushort			NumChars;

	BlockLen;

	// Layer sub-block header
	if (iCurImage->io.read(iCurImage->io.handle, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;
	if (Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(&iCurImage->io);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_LAYER_BLOCK)
		return IL_FALSE;


	if (Header.MajorVersion == 3) {
		iCurImage->io.seek(iCurImage->io.handle, 256, IL_SEEK_CUR);  // We don't care about the name of the layer.
		iCurImage->io.read(iCurImage->io.handle, &LayerInfo, sizeof(LayerInfo), 1);
		if (iCurImage->io.read(iCurImage->io.handle, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
	}
	else {  // Header.MajorVersion >= 4
		ChunkSize = GetLittleUInt(&iCurImage->io);
		NumChars = GetLittleUShort(&iCurImage->io);
		iCurImage->io.seek(iCurImage->io.handle, NumChars, IL_SEEK_CUR);  // We don't care about the layer's name.

		ChunkSize -= (2 + 4 + NumChars);

		if (iCurImage->io.read(iCurImage->io.handle, &LayerInfo, IL_MIN(sizeof(LayerInfo), ChunkSize), 1) != 1)
			return IL_FALSE;

		// Can have new entries in newer versions of the spec (5.0).
		Padding = (ChunkSize) - sizeof(LayerInfo);
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(&iCurImage->io);
		if (iCurImage->io.read(iCurImage->io.handle, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4) - sizeof(Bitmap);
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);
	}


	Channels = (ILubyte**)ialloc(sizeof(ILubyte*) * Bitmap.NumChannels);
	if (Channels == NULL) {
		return IL_FALSE;
	}

	NumChannels = Bitmap.NumChannels;

	for (i = 0; i < NumChannels; i++) {
		Channels[i] = GetChannel();
		if (Channels[i] == NULL) {
			for (j = 0; j < i; j++)
				ifree(Channels[j]);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}


ILboolean ReadAlphaBlock(ILuint BlockLen)
{
	BLOCKHEAD		Block;
	ALPHAINFO_CHUNK	AlphaInfo;
	ALPHA_CHUNK		AlphaChunk;
	ILushort		NumAlpha, StringSize;
	ILuint			ChunkSize, Padding;

	if (Header.MajorVersion == 3) {
		NumAlpha = GetLittleUShort(&iCurImage->io);
	}
	else {
		ChunkSize = GetLittleUInt(&iCurImage->io);
		NumAlpha = GetLittleUShort(&iCurImage->io);
		Padding = (ChunkSize - 4 - 2);
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);
	}

	// Alpha channel header
	if (iCurImage->io.read(iCurImage->io.handle, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;
	if (Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(&iCurImage->io);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_ALPHA_CHANNEL_BLOCK)
		return IL_FALSE;


	if (Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(&iCurImage->io);
		StringSize = GetLittleUShort(&iCurImage->io);
		iCurImage->io.seek(iCurImage->io.handle, StringSize, IL_SEEK_CUR);
		if (iCurImage->io.read(iCurImage->io.handle, &AlphaInfo, sizeof(AlphaInfo), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - 2 - StringSize - sizeof(AlphaInfo));
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(&iCurImage->io);
		if (iCurImage->io.read(iCurImage->io.handle, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - sizeof(AlphaChunk));
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);
	}
	else {
		iCurImage->io.seek(iCurImage->io.handle, 256, IL_SEEK_CUR);
		iCurImage->io.read(iCurImage->io.handle, &AlphaInfo, sizeof(AlphaInfo), 1);
		if (iCurImage->io.read(iCurImage->io.handle, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
	}


	/*Alpha = (ILubyte*)ialloc(AlphaInfo.AlphaRect.x2 * AlphaInfo.AlphaRect.y2);
	if (Alpha == NULL) {
		return IL_FALSE;
	}*/


	Alpha = GetChannel();
	if (Alpha == NULL)
		return IL_FALSE;

	return IL_TRUE;
}


ILubyte *GetChannel()
{
	BLOCKHEAD		Block;
	CHANNEL_CHUNK	Channel;
	ILubyte			*CompData, *Data;
	ILuint			ChunkSize, Padding;

	if (iCurImage->io.read(iCurImage->io.handle, &Block, 1, sizeof(Block)) != sizeof(Block))
		return NULL;
	if (Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(&iCurImage->io);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return NULL;
	}
	if (Block.BlockID != PSP_CHANNEL_BLOCK) {
		ilSetError(IL_ILLEGAL_FILE_VALUE);
		return NULL;
	}


	if (Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(&iCurImage->io);
		if (iCurImage->io.read(iCurImage->io.handle, &Channel, sizeof(Channel), 1) != 1)
			return NULL;

		Padding = (ChunkSize - 4) - sizeof(Channel);
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);
	}
	else {
		if (iCurImage->io.read(iCurImage->io.handle, &Channel, sizeof(Channel), 1) != 1)
			return NULL;
	}


	CompData = (ILubyte*)ialloc(Channel.CompLen);
	Data = (ILubyte*)ialloc(AttChunk.Width * AttChunk.Height);
	if (CompData == NULL || Data == NULL) {
		ifree(Data);
		ifree(CompData);
		return NULL;
	}

	if (iCurImage->io.read(iCurImage->io.handle, CompData, 1, Channel.CompLen) != Channel.CompLen) {
		ifree(CompData);
		ifree(Data);
		return NULL;
	}

	switch (AttChunk.Compression)
	{
		case PSP_COMP_NONE:
			ifree(Data);
			return CompData;
			break;

		case PSP_COMP_RLE:
			if (!UncompRLE(CompData, Data, Channel.CompLen)) {
				ifree(CompData);
				ifree(Data);
				return IL_FALSE;
			}
			break;

		default:
			ifree(CompData);
			ifree(Data);
			ilSetError(IL_INVALID_FILE_HEADER);
			return NULL;
	}

	ifree(CompData);

	return Data;
}


ILboolean UncompRLE(ILubyte *CompData, ILubyte *Data, ILuint CompLen)
{
	ILubyte	Run, Colour;
	ILint	i, /*x, y,*/ Count/*, Total = 0*/;

	/*for (y = 0; y < AttChunk.Height; y++) {
		for (x = 0, Count = 0; x < AttChunk.Width; ) {
			Run = *CompData++;
			if (Run > 128) {
				Run -= 128;
				Colour = *CompData++;
				memset(Data, Colour, Run);
				Data += Run;
				Count += 2;
			}
			else {
				memcpy(Data, CompData, Run);
				CompData += Run;
				Data += Run;
				Count += Run;
			}
			x += Run;
		}

		Total += Count;

		if (Count % 4) {  // Has to be on a 4-byte boundary.
			CompData += (4 - (Count % 4)) % 4;
			Total += (4 - (Count % 4)) % 4;
		}

		if (Total >= CompLen)
			return IL_FALSE;
	}*/

	for (i = 0, Count = 0; i < (ILint)CompLen; ) {
		Run = *CompData++;
		i++;
		if (Run > 128) {
			Run -= 128;
			Colour = *CompData++;
			i++;
			memset(Data, Colour, Run);
		}
		else {
			memcpy(Data, CompData, Run);
			CompData += Run;
			i += Run;
		}
		Data += Run;
		Count += Run;
	}

	return IL_TRUE;
}


ILboolean ReadPalette(ILuint BlockLen)
{
	ILuint ChunkSize, PalCount, Padding;

	if (Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(&iCurImage->io);
		PalCount = GetLittleUInt(&iCurImage->io);
		Padding = (ChunkSize - 4 - 4);
		if (Padding > 0)
			iCurImage->io.seek(iCurImage->io.handle, Padding, IL_SEEK_CUR);
	}
	else {
		PalCount = GetLittleUInt(&iCurImage->io);
	}

	Pal.PalSize = PalCount * 4;
	Pal.PalType = IL_PAL_BGRA32;
	Pal.Palette = (ILubyte*)ialloc(Pal.PalSize);
	if (Pal.Palette == NULL)
		return IL_FALSE;

	if (iCurImage->io.read(iCurImage->io.handle, Pal.Palette, Pal.PalSize, 1) != 1) {
		ifree(Pal.Palette);
		return IL_FALSE;
	}

	return IL_TRUE;
}


ILboolean AssembleImage()
{
	ILuint Size, i, j;

	Size = AttChunk.Width * AttChunk.Height;

	if (NumChannels == 1) {
		ilTexImage(AttChunk.Width, AttChunk.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
		for (i = 0; i < Size; i++) {
			iCurImage->Data[i] = Channels[0][i];
		}

		if (Pal.Palette) {
			iCurImage->Format = IL_COLOUR_INDEX;
			iCurImage->Pal.PalSize = Pal.PalSize;
			iCurImage->Pal.PalType = Pal.PalType;
			iCurImage->Pal.Palette = Pal.Palette;
		}
	}
	else {
		if (Alpha) {
			ilTexImage(AttChunk.Width, AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 4) {
				iCurImage->Data[j  ] = Channels[0][i];
				iCurImage->Data[j+1] = Channels[1][i];
				iCurImage->Data[j+2] = Channels[2][i];
				iCurImage->Data[j+3] = Alpha[i];
			}
		}

		else if (NumChannels == 4) {

			ilTexImage(AttChunk.Width, AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);

			for (i = 0, j = 0; i < Size; i++, j += 4) {

				iCurImage->Data[j  ] = Channels[0][i];

				iCurImage->Data[j+1] = Channels[1][i];

				iCurImage->Data[j+2] = Channels[2][i];

				iCurImage->Data[j+3] = Channels[3][i];

			}

		}
		else if (NumChannels == 3) {
			ilTexImage(AttChunk.Width, AttChunk.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 3) {
				iCurImage->Data[j  ] = Channels[0][i];
				iCurImage->Data[j+1] = Channels[1][i];
				iCurImage->Data[j+2] = Channels[2][i];
			}
		}
		else
			return IL_FALSE;
	}

	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}


ILboolean Cleanup()
{
	ILuint	i;

	if (Channels) {
		for (i = 0; i < NumChannels; i++) {
			ifree(Channels[i]);
		}
		ifree(Channels);
	}

	if (Alpha) {
		ifree(Alpha);
	}

	Channels = NULL;
	Alpha = NULL;
	Pal.Palette = NULL;

	return IL_TRUE;
}



#endif//IL_NO_PSP
