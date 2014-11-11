//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/15/2009
//
// Filename: src-IL/src/il_rot.c
//
// Description: Reads from a Homeworld 2 - Relic Texture (.rot) file.
//
//-----------------------------------------------------------------------------

// @TODO:
// Note: I am not certain about which is DXT3 and which is DXT5.  According to
//  http://forums.relicnews.com/showthread.php?t=20512, DXT3 is 1030, and DXT5
//  is 1029.  However, neither way seems to work quite right for the alpha.

#include "il_internal.h"
#ifndef IL_NO_ROT
#include "il_dds.h"

static ILboolean iLoadRotInternal(ILimage *);

#define ROT_RGBA32	1024
#define ROT_DXT1		1028
#define ROT_DXT3		1029
#define ROT_DXT5		1030

#include "pack_push.h"
typedef struct {
	ILuint 	FORM;
	ILuint  FormLength;
	ILuint  FormName;
} FORM_HEAD;

typedef struct {
	FORM_HEAD 	FormHead;
	ILuint  		Width, Height, Format;
} ROT_HEAD;
#include "pack_pop.h"

static ILboolean iGetFormHeader(SIO *io, FORM_HEAD *Head) {
	if (SIOread(io, Head, 1, sizeof(*Head)) != sizeof(*Head))
		return IL_FALSE;

	BigUInt(&Head->FORM);
	BigUInt(&Head->FormLength);
	BigUInt(&Head->FormName);

	return memcmp(&Head->FORM, "FORM", 4) == 0;
}

static ILboolean iIsValidRot(SIO *io) {
	ILuint 		Start = SIOtell(io);
	ROT_HEAD 	Head;
	ILuint 		Read = SIOread(io, &Head, 1, sizeof(Head));

	SIOseek(io, Start, IL_SEEK_SET);

	BigUInt(&Head.FormHead.FORM);
	BigUInt(&Head.FormHead.FormLength);
	BigUInt(&Head.FormHead.FormName);

	return Read == sizeof(Head) 
	    && !memcmp(&Head.FormHead.FORM, "FORM", 4)
			&& Head.FormHead.FormLength == 0x14 
			&& !memcmp(&Head.FormHead.FormName, "HEAD", 4);
}

// Internal function used to load the ROT.
static ILboolean iLoadRotInternal(ILimage *Image) {
	ILubyte   Channels;
	ILuint    CompSize;
	ILuint		MipSize, MipWidth, MipHeight;
	ILenum		FormatIL;
	ILubyte		*CompData = NULL;
	FORM_HEAD Form;
	ILimage * BaseImage = Image;
	ROT_HEAD 	Head;
	SIO * 		io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	if (SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head))
		return IL_FALSE;

	BigUInt(&Head.FormHead.FORM);
	BigUInt(&Head.FormHead.FormLength);
	BigUInt(&Head.FormHead.FormName);
	UInt   (&Head.Width);
	UInt   (&Head.Height);
	UInt   (&Head.Format);

	// The first entry in the file must be 'FORM', 0x20 in a big endian integer and then 'HEAD'.
	if ( memcmp(&Head.FormHead.FORM, "FORM", 4) 
		|| Head.FormHead.FormLength != 0x14 
		|| memcmp(&Head.FormHead.FormName, "HEAD", 4)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (Head.Width == 0 || Head.Height == 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	//@TODO: More formats.
	switch (Head.Format)	{
		case ROT_RGBA32:  // 32-bit RGBA format
			Channels = 4;
			FormatIL = IL_RGBA;
			break;

		case ROT_DXT1:  // DXT1 (no alpha)
			Channels = 4;
			FormatIL = IL_RGBA;
			break;

		case ROT_DXT3:  // DXT3
		case ROT_DXT5:  // DXT5
			Channels = 4;
			FormatIL = IL_RGBA;
			/* not used?
			// Allocates the maximum needed (the first width/height given in the file).
			CompSize = ((Head.Width + 3) / 4) * ((Head.Height + 3) / 4) * 16;
			CompData = ialloc(CompSize);
			if (CompData == NULL)
				return IL_FALSE; */
			break;

		default:
			iSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	//@TODO: Find out what this is.
	GetLittleUInt(io);  // Skip this for the moment.  This appears to be the number of channels.

	// Next comes 'FORM', a length and 'MIPS'.
	//@TODO: Not sure if the FormLen has to be anything specific here.
	if ( !iGetFormHeader(io, &Form)
		|| memcmp(&Form.FormName, "MIPS", 4)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	//@TODO: Can these mipmap levels be in any order?  Some things may be easier if the answer is no.
	do {
		if ( !iGetFormHeader(io, &Form)
		  || memcmp(&Form.FormName, "MLVL", 4)) {
			if (!BaseImage) {
				iSetError(IL_INVALID_FILE_HEADER);
				return IL_FALSE;
			}
			break;
		}

		// Next is the mipmap attributes (level number, width, height and length)
		/* MipLevel = */ GetLittleUInt(io);
		MipWidth 	= GetLittleUInt(io);
		MipHeight = GetLittleUInt(io);
		MipSize 	= GetLittleUInt(io);  // This is the same as the previous size listed -20 (for attributes).

		// Lower level mipmaps cannot be larger than the main image.
		if (MipWidth > Head.Width || MipHeight > Head.Height /* || MipSize > CompSize */) {
			iSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}

		// Just create our images here.
		if (!BaseImage) {
			if (!iTexImage(Image, MipWidth, MipHeight, 1, Channels, FormatIL, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			BaseImage = Image;
		}	else {
			Image->Mipmaps = iNewImageFull(MipWidth, MipHeight, 1, Channels, FormatIL, IL_UNSIGNED_BYTE, NULL);
			Image = Image->Mipmaps;
		}

		switch (Head.Format) {
			case ROT_RGBA32:  // 32-bit RGBA format
				if (SIOread(io, Image->Data, Image->SizeOfData, 1) != 1)
					return IL_FALSE;
				break;

			case ROT_DXT1:
				// Allocates the size of the compressed data.
				CompSize = ((MipWidth + 3) / 4) * ((MipHeight + 3) / 4) * 8;
				if (CompSize != MipSize) {
					iSetError(IL_INVALID_FILE_HEADER);
					return IL_FALSE;
				}

				CompData = ialloc(CompSize);
				if (CompData == NULL)
					return IL_FALSE;

				// Read in the DXT1 data...
				if (SIOread(io, CompData, CompSize, 1) != 1) {
					ifree(CompData);
					return IL_FALSE;
				}

				// ...and decompress it.
				if (!DecompressDXT1(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}

				if (iIsEnabled(IL_KEEP_DXTC_DATA)) {
					Image->DxtcSize   = CompSize;
					Image->DxtcData   = CompData;
					Image->DxtcFormat = IL_DXT1;
					CompData = NULL;
				}
				break;

			case ROT_DXT3:
				// Allocates the size of the compressed data.
				CompSize = ((MipWidth + 3) / 4) * ((MipHeight + 3) / 4) * 16;
				if (CompSize != MipSize) {
					iSetError(IL_INVALID_FILE_HEADER);
					return IL_FALSE;
				}

				CompData = ialloc(CompSize);
				if (CompData == NULL)
					return IL_FALSE;

				// Read in the DXT3 data...
				if (SIOread(io, CompData, MipSize, 1) != 1) {
					ifree(CompData);
					return IL_FALSE;
				}

				// ...and decompress it.
				if (!DecompressDXT3(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}

				if (iIsEnabled(IL_KEEP_DXTC_DATA)) {
					Image->DxtcSize = CompSize;
					Image->DxtcData = CompData;
					Image->DxtcFormat = IL_DXT3;
					CompData = NULL;
				}
				break;

			case ROT_DXT5:
				// Allocates the size of the compressed data.
				CompSize = ((MipWidth + 3) / 4) * ((MipHeight + 3) / 4) * 16;
				if (CompSize != MipSize) {
					iSetError(IL_INVALID_FILE_HEADER);
					return IL_FALSE;
				}
				CompData = ialloc(CompSize);
				if (CompData == NULL)
					return IL_FALSE;

				// Read in the DXT5 data...
				if (SIOread(io, CompData, MipSize, 1) != 1){
					ifree(CompData);
					return IL_FALSE;
				}

				// ...and decompress it.
				if (!DecompressDXT5(Image, CompData)) {
					ifree(CompData);
					return IL_FALSE;
				}

				// Keeps a copy
				if (iIsEnabled(IL_KEEP_DXTC_DATA)) {
					Image->DxtcSize   = CompSize;
					Image->DxtcData   = CompData;
					Image->DxtcFormat = IL_DXT5;
					CompData = NULL;
				}
				break;
		}
		ifree(CompData);  // Free it if it was not saved.
	} while (!SIOeof(io));  //@TODO: Is there any other condition that should end this?

	return IL_TRUE;
}

static ILconst_string iFormatExtsROT[] = { 
	IL_TEXT("rot"), 
	NULL 
};

ILformat iFormatROT = { 
	/* .Validate = */ iIsValidRot, 
  /* .Load     = */ iLoadRotInternal, 
	/* .Save     = */ NULL, 
	/* .Exts     = */ iFormatExtsROT
};

#endif//IL_NO_ROT

