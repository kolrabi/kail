//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_mdl.c
//
// Description: Reads a Half-Life model file (.mdl).
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_MDL
#include "il_mdl.h"

// Internal function to get the header and check it.
static ILboolean iIsValidMdl(SIO *io)
{
	ILuint 		Pos = SIOtell(io);
	MDL_HEAD 	Head;
	ILuint  	Read = SIOread(io, &Head, 1, sizeof(Head));

	UInt(&Head.Version);

	SIOseek(io, Pos, IL_SEEK_SET);

	return Read == sizeof(Head) && !memcmp(Head.Magic, "IDST", 4) && Head.Version == 10;
}

static ILboolean iLoadMdlInternal(ILimage *Image)
{
	ILimage *	BaseImage 	= NULL;
	MDL_HEAD Head;
	TEX_INFO TexInfo;
	ILuint ImageNum;
	SIO *io;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	if (SIOread(io, &Head, 1, sizeof(Head)) != sizeof(Head)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	UInt(&Head.Version);

	// Skips the actual model header.
	SIOseek(io, 172, IL_SEEK_CUR);

	if (SIOread(io, &TexInfo, 1, sizeof(TexInfo)) != sizeof(TexInfo)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	UInt(&TexInfo.NumTex);
	UInt(&TexInfo.TexOff);
	UInt(&TexInfo.TexDataOff);

	if (TexInfo.NumTex == 0 || TexInfo.TexOff == 0 || TexInfo.TexDataOff == 0) {
		iSetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	SIOseek(io, TexInfo.TexOff, IL_SEEK_SET);

	for (ImageNum = 0; ImageNum < TexInfo.NumTex; ImageNum++) {
		TEX_HEAD	TexHead;
        ILuint      Position;
        ILubyte *   TempPal;

		if (SIOread(io, &TexHead, 1, sizeof(TexHead)) != sizeof(TexHead)) {
			return IL_FALSE;
		}

		UInt(&TexHead.Flags);
		UInt(&TexHead.Width);
		UInt(&TexHead.Height);
		UInt(&TexHead.Offset);

		Position = SIOtell(io);

		if (TexHead.Offset == 0) {
			iSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		if (!BaseImage) {
			iTexImage(Image, TexHead.Width, TexHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL);
			Image->Origin = IL_ORIGIN_LOWER_LEFT;
			BaseImage 		= Image;
		} else {
			Image->Next 	= ilNewImage(TexHead.Width, TexHead.Height, 1, 1, 1);
			Image 				= Image->Next;
			Image->Format = IL_COLOUR_INDEX;
			Image->Type 	= IL_UNSIGNED_BYTE;
		}

		TempPal	= (ILubyte*)ialloc(768);
		if (TempPal == NULL) {
			return IL_FALSE;
		}

		Image->Pal.Palette = TempPal;
		Image->Pal.PalSize = 768;
		Image->Pal.PalType = IL_PAL_RGB24;

		SIOseek(io, TexHead.Offset, IL_SEEK_SET);
		
		if (SIOread(io, Image->Data, TexHead.Width * TexHead.Height, 1) != 1)
			return IL_FALSE;

		if (SIOread(io, Image->Pal.Palette, 1, 768) != 768)
			return IL_FALSE;

		SIOseek(io, Position, IL_SEEK_SET);
	}

	return IL_TRUE;
}

ILconst_string iFormatExtsMDL[] = { 
  IL_TEXT("mdl"), 
  NULL 
};

ILformat iFormatMDL = { 
  /* .Validate = */ iIsValidMdl, 
  /* .Load     = */ iLoadMdlInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsMDL
};

#endif//IL_NO_MDL
