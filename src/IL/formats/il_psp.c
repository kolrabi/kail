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

// Make these global, since they contain most of the image information.
// BP: y'know what? let's not make these global, global state is bad mmkay?
typedef struct {
	GENATT_CHUNK	AttChunk;
	PSPHEAD				Header;
	ILuint				NumChannels;
	ILubyte	**		Channels ;
	ILubyte *			Alpha;
	ILpal					Pal;
	SIO *         io;
	ILimage *     Image;
} PSP_CTX;

// Function definitions
static ILboolean	iLoadPspInternal(ILimage *);
static ILboolean	iCheckPsp(PSPHEAD *);
static ILboolean	ReadGenAttributes(PSP_CTX *);
static ILboolean	ParseChunks(PSP_CTX *);
static ILboolean	AssembleImage(PSP_CTX *);
static ILboolean	Cleanup(PSP_CTX *);
static ILboolean	ReadLayerBlock(PSP_CTX *);
static ILboolean	ReadAlphaBlock(PSP_CTX *);
static ILboolean	ReadPalette(PSP_CTX *);
static ILubyte* 	GetChannel(PSP_CTX *);
static ILboolean	UncompRLE(ILubyte *CompData, ILubyte *Data, ILuint CompLen);

// Global constants
static const ILubyte PSPSignature[32] = {
	0x50, 0x61, 0x69, 0x6E, 0x74, 0x20, 0x53, 0x68, 0x6F, 0x70, 0x20, 0x50, 0x72, 0x6F, 0x20, 0x49,
	0x6D, 0x61, 0x67, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x0A, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const ILubyte GenAttHead[4] = {
	0x7E, 0x42, 0x4B, 0x00
};

// Internal function used to get the Psp header from the current file.
static ILboolean iGetPspHead(SIO *io, PSPHEAD *Header)
{
	if (!SIOread(io, Header, sizeof(*Header), 1))
		return IL_FALSE;

	UShort(&Header->MajorVersion);
	UShort(&Header->MinorVersion);
	
	return IL_TRUE;
}

// Internal function to get the header and check it.
static ILboolean iIsValidPsp(SIO *io)
{
	PSPHEAD 	Header;
	ILuint 		Start 	= SIOtell(io);
	ILboolean	GotHead = iGetPspHead(io, &Header);

	SIOseek(io, Start, IL_SEEK_SET);

	return GotHead && iCheckPsp(&Header);
}

// Internal function used to check if the HEADER is a valid Psp header.
ILboolean iCheckPsp(PSPHEAD *Header)
{
	if (memcmp(Header->FileSig, "Paint Shop Pro Image File\n\x1a", 7))
		return IL_FALSE;

	if (Header->MajorVersion < 3 || Header->MajorVersion > 5)
		return IL_FALSE;

	if (Header->MinorVersion != 0)
		return IL_FALSE;

	return IL_TRUE;
}

// Internal function used to load the PSP.
static ILboolean iLoadPspInternal(ILimage *Image) {
	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	PSP_CTX ctx;

	ctx.Channels 		= NULL;
	ctx.Alpha 			= NULL;
	ctx.Pal.Palette = NULL;
	ctx.io          = &Image->io;
	ctx.Image       = Image;

	if (!iGetPspHead(ctx.io, &ctx.Header))
		return IL_FALSE;

	if (!iCheckPsp(&ctx.Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadGenAttributes(&ctx))
		return IL_FALSE;

	if (!ParseChunks(&ctx))
		return IL_FALSE;

	if (!AssembleImage(&ctx))
		return IL_FALSE;

	Cleanup(&ctx);
	return ilFixImage();
}

static ILboolean ReadGenAttributes(PSP_CTX *ctx)
{
	BLOCKHEAD		AttHead;
	ILint				Padding;
	ILuint			ChunkLen;

	if (SIOread(ctx->io, &AttHead, sizeof(AttHead), 1) != 1)
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

	ChunkLen = GetLittleUInt(ctx->io);
	if (ctx->Header.MajorVersion != 3)
		ChunkLen -= 4;

	if (SIOread(ctx->io, &ctx->AttChunk, IL_MIN(sizeof(ctx->AttChunk), ChunkLen), 1) != 1)
		return IL_FALSE;

	// Can have new entries in newer versions of the spec (4.0).
	Padding = (ChunkLen) - sizeof(ctx->AttChunk);
	if (Padding > 0)
		SIOseek(ctx->io, Padding, IL_SEEK_CUR);

	// @TODO:  Anything but 24 not supported yet...
	if (ctx->AttChunk.BitDepth != 24 && ctx->AttChunk.BitDepth != 8) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO;  Add support for compression...
	if (ctx->AttChunk.Compression != PSP_COMP_NONE && ctx->AttChunk.Compression != PSP_COMP_RLE) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO: Check more things in the general attributes chunk here.

	return IL_TRUE;
}


static ILboolean ParseChunks(PSP_CTX *ctx)
{
	BLOCKHEAD	Block;
	ILuint		Pos;

	do {
		if (SIOread(ctx->io, &Block, 1, sizeof(Block)) != sizeof(Block)) {
			ilGetError();  // Get rid of the erroneous IL_FILE_READ_ERROR.
			return IL_TRUE;
		}

		if (ctx->Header.MajorVersion == 3) {
			Block.BlockLen = GetLittleUInt(ctx->io);
		}	else {
			UInt(&Block.BlockLen);
		}

		if ( Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 
			|| Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00 ) {
				return IL_TRUE;
		}

		UShort(&Block.BlockID);
		UInt(&Block.BlockLen);

		Pos = SIOtell(ctx->io);

		switch (Block.BlockID)
		{
			case PSP_LAYER_START_BLOCK:
				if (!ReadLayerBlock(ctx))
					return IL_FALSE;
				break;

			case PSP_ALPHA_BANK_BLOCK:
				if (!ReadAlphaBlock(ctx))
					return IL_FALSE;
				break;

			case PSP_COLOR_BLOCK:
				if (!ReadPalette(ctx))
					return IL_FALSE;
				break;

			// Gets done in the next iseek, so this is now commented out.
			//default:
				//SIOseek(ctx->io, Block.BlockLen, IL_SEEK_CUR);
		}

		// Skip to next block just in case we didn't read the entire block.
		SIOseek(ctx->io, Pos + Block.BlockLen, IL_SEEK_SET);

		// @TODO: Do stuff here.

	} while (1);

	return IL_TRUE;
}

static ILboolean ReadLayerBlock(PSP_CTX *ctx) {
	BLOCKHEAD			Block;
	LAYERINFO_CHUNK		LayerInfo;
	LAYERBITMAP_CHUNK	Bitmap;
	ILuint				ChunkSize, Padding, i, j;
	ILushort			NumChars;

	// Layer sub-block header
	if (SIOread(ctx->io, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;

	if (ctx->Header.MajorVersion == 3) {
		Block.BlockLen = GetLittleUInt(ctx->io);
	}	else {
		UInt(&Block.BlockLen);
	}

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_LAYER_BLOCK)
		return IL_FALSE;


	if (ctx->Header.MajorVersion == 3) {
		SIOseek(ctx->io, 256, IL_SEEK_CUR);  // We don't care about the name of the layer.
		SIOread(ctx->io, &LayerInfo, sizeof(LayerInfo), 1);
		if (SIOread(ctx->io, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
	}
	else {  // ctx->Header.MajorVersion >= 4
		ChunkSize = GetLittleUInt(ctx->io);
		NumChars = GetLittleUShort(ctx->io);
		SIOseek(ctx->io, NumChars, IL_SEEK_CUR);  // We don't care about the layer's name.

		ChunkSize -= (2 + 4 + NumChars);

		if (SIOread(ctx->io, &LayerInfo, IL_MIN(sizeof(LayerInfo), ChunkSize), 1) != 1)
			return IL_FALSE;

		// Can have new entries in newer versions of the spec (5.0).
		Padding = (ChunkSize) - sizeof(LayerInfo);
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(ctx->io);
		if (SIOread(ctx->io, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4) - sizeof(Bitmap);
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);
	}


	ctx->Channels = (ILubyte**)ialloc(sizeof(ILubyte*) * Bitmap.NumChannels);
	if (ctx->Channels == NULL) {
		return IL_FALSE;
	}

	ctx->NumChannels = Bitmap.NumChannels;

	for (i = 0; i < ctx->NumChannels; i++) {
		ctx->Channels[i] = GetChannel(ctx);
		if (ctx->Channels[i] == NULL) {
			for (j = 0; j < i; j++)
				ifree(ctx->Channels[j]);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}

static ILboolean ReadAlphaBlock(PSP_CTX *ctx) {
	BLOCKHEAD				Block;
	ALPHAINFO_CHUNK	AlphaInfo;
	ALPHA_CHUNK			AlphaChunk;
	ILushort				StringSize;
	ILuint					ChunkSize, Padding;

	if (ctx->Header.MajorVersion == 3) {
		/* ILuint NumAlpha = */ GetLittleUShort(ctx->io);
	}
	else {
		ChunkSize = GetLittleUInt(ctx->io);
		/* NumAlpha = */ GetLittleUShort(ctx->io);
		Padding = (ChunkSize - 4 - 2);
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);
	}

	// Alpha channel header
	if (SIOread(ctx->io, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;

	if (ctx->Header.MajorVersion == 3) {
		Block.BlockLen = GetLittleUInt(ctx->io);
	}	else {
		UInt(&Block.BlockLen);
	}

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_ALPHA_CHANNEL_BLOCK)
		return IL_FALSE;

	if (ctx->Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(ctx->io);
		StringSize = GetLittleUShort(ctx->io);
		SIOseek(ctx->io, StringSize, IL_SEEK_CUR);
		if (SIOread(ctx->io, &AlphaInfo, sizeof(AlphaInfo), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - 2 - StringSize - sizeof(AlphaInfo));
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(ctx->io);
		if (SIOread(ctx->io, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - sizeof(AlphaChunk));
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);
	}
	else {
		SIOseek(ctx->io, 256, IL_SEEK_CUR);
		SIOread(ctx->io, &AlphaInfo, sizeof(AlphaInfo), 1);
		if (SIOread(ctx->io, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
	}


	/*Alpha = (ILubyte*)ialloc(AlphaInfo.AlphaRect.x2 * AlphaInfo.AlphaRect.y2);
	if (Alpha == NULL) {
		return IL_FALSE;
	}*/


	ctx->Alpha = GetChannel(ctx);
	if (ctx->Alpha == NULL)
		return IL_FALSE;

	return IL_TRUE;
}

static ILubyte *GetChannel(PSP_CTX *ctx)
{
	BLOCKHEAD		Block;
	CHANNEL_CHUNK	Channel;
	ILubyte			*CompData, *Data;
	ILuint			ChunkSize, Padding;

	if (SIOread(ctx->io, &Block, 1, sizeof(Block)) != sizeof(Block))
		return NULL;

	if (ctx->Header.MajorVersion == 3) {
		Block.BlockLen = GetLittleUInt(ctx->io);
	}	else {
		UInt(&Block.BlockLen);
	}

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return NULL;
	}
	if (Block.BlockID != PSP_CHANNEL_BLOCK) {
		ilSetError(IL_ILLEGAL_FILE_VALUE);
		return NULL;
	}


	if (ctx->Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(ctx->io);
		if (SIOread(ctx->io, &Channel, sizeof(Channel), 1) != 1)
			return NULL;

		Padding = (ChunkSize - 4) - sizeof(Channel);
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);
	}
	else {
		if (SIOread(ctx->io, &Channel, sizeof(Channel), 1) != 1)
			return NULL;
	}


	CompData = (ILubyte*)ialloc(Channel.CompLen);
	Data = (ILubyte*)ialloc(ctx->AttChunk.Width * ctx->AttChunk.Height);
	if (CompData == NULL || Data == NULL) {
		ifree(Data);
		ifree(CompData);
		return NULL;
	}

	if (SIOread(ctx->io, CompData, 1, Channel.CompLen) != Channel.CompLen) {
		ifree(CompData);
		ifree(Data);
		return NULL;
	}

	switch (ctx->AttChunk.Compression)
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

	/*for (y = 0; y < ctx->AttChunk.Height; y++) {
		for (x = 0, Count = 0; x < ctx->AttChunk.Width; ) {
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

static ILboolean ReadPalette(PSP_CTX *ctx) {
	ILuint ChunkSize, PalCount, Padding;

	if (ctx->Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(ctx->io);
		PalCount = GetLittleUInt(ctx->io);
		Padding = (ChunkSize - 4 - 4);
		if (Padding > 0)
			SIOseek(ctx->io, Padding, IL_SEEK_CUR);
	}
	else {
		PalCount = GetLittleUInt(ctx->io);
	}

	ctx->Pal.PalSize = PalCount * 4;
	ctx->Pal.PalType = IL_PAL_BGRA32;
	ctx->Pal.Palette = (ILubyte*)ialloc(ctx->Pal.PalSize);
	if (ctx->Pal.Palette == NULL)
		return IL_FALSE;

	if (SIOread(ctx->io, ctx->Pal.Palette, ctx->Pal.PalSize, 1) != 1) {
		ifree(ctx->Pal.Palette);
		return IL_FALSE;
	}

	return IL_TRUE;
}

static ILboolean AssembleImage(PSP_CTX *ctx) {
	ILuint Size, i, j;

	Size = ctx->AttChunk.Width * ctx->AttChunk.Height;

	if (ctx->NumChannels == 1) {
		ilTexImage_(ctx->Image, ctx->AttChunk.Width, ctx->AttChunk.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
		for (i = 0; i < Size; i++) {
			ctx->Image->Data[i] = ctx->Channels[0][i];
		}

		if (ctx->Pal.Palette) {
			ctx->Image->Format = IL_COLOUR_INDEX;
			ctx->Image->Pal.PalSize = ctx->Pal.PalSize;
			ctx->Image->Pal.PalType = ctx->Pal.PalType;
			ctx->Image->Pal.Palette = ctx->Pal.Palette;
		}
	}
	else {
		if (ctx->Alpha) {
			ilTexImage_(ctx->Image, ctx->AttChunk.Width, ctx->AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 4) {
				ctx->Image->Data[j  ] = ctx->Channels[0][i];
				ctx->Image->Data[j+1] = ctx->Channels[1][i];
				ctx->Image->Data[j+2] = ctx->Channels[2][i];
				ctx->Image->Data[j+3] = ctx->Alpha[i];
			}
		}

		else if (ctx->NumChannels == 4) {

			ilTexImage_(ctx->Image, ctx->AttChunk.Width, ctx->AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);

			for (i = 0, j = 0; i < Size; i++, j += 4) {
				ctx->Image->Data[j  ] = ctx->Channels[0][i];
				ctx->Image->Data[j+1] = ctx->Channels[1][i];
				ctx->Image->Data[j+2] = ctx->Channels[2][i];
				ctx->Image->Data[j+3] = ctx->Channels[3][i];
			}

		}
		else if (ctx->NumChannels == 3) {
			ilTexImage_(ctx->Image, ctx->AttChunk.Width, ctx->AttChunk.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 3) {
				ctx->Image->Data[j  ] = ctx->Channels[0][i];
				ctx->Image->Data[j+1] = ctx->Channels[1][i];
				ctx->Image->Data[j+2] = ctx->Channels[2][i];
			}
		}
		else
			return IL_FALSE;
	}

	ctx->Image->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}

static ILboolean Cleanup(PSP_CTX *ctx) {
	ILuint	i;

	if (ctx->Channels) {
		for (i = 0; i < ctx->NumChannels; i++) {
			ifree(ctx->Channels[i]);
		}
		ifree(ctx->Channels);
	}

	if (ctx->Alpha) {
		ifree(ctx->Alpha);
	}

	ctx->Channels 		= NULL;
	ctx->Alpha 				= NULL;
	ctx->Pal.Palette 	= NULL;

	return IL_TRUE;
}

ILconst_string iFormatExtsPSP[] = { 
  IL_TEXT("psp"), 
  NULL 
};

ILformat iFormatPSP = { 
  .Validate = iIsValidPsp, 
  .Load     = iLoadPspInternal, 
  .Save     = NULL, 
  .Exts     = iFormatExtsPSP
};

#endif//IL_NO_PSP
