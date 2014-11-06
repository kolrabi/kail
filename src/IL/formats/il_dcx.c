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

static ILboolean 
iIsValidDcx(SIO *io) {
	ILuint Signature;
	ILuint Start = SIOtell(io);
	ILuint Read = SIOread(io, &Signature, 1, 4);

  SIOseek(io, Start, IL_SEEK_SET);

	if (Read != 4)
		return IL_FALSE;

	UInt(&Signature);

	return (Signature == 987654321);
}


// Internal function used to load the .dcx.
static ILboolean
iLoadDcxInternal(ILimage *TargetImage)
{
	PCXHEAD	Header;
	ILuint	Signature, i, Entries[1024], Num = 0;
	ILimage	*Prev = TargetImage;
	ILimage *Image = TargetImage;
  SIO *    io;

	if (TargetImage == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &TargetImage->io;

	if (!iIsValidDcx(io))
		return IL_FALSE;

	SIOread(io, &Signature, 1, 4);

	do {
		if (SIOread(io, &Entries[Num], 1, 4) != 4)
			return IL_FALSE;
		UInt(&Entries[Num]);
		Num++;
	} while (Entries[Num-1] != 0 && Num < 1024);
	Num--;

	for (i = 0; i < Num; i++) {
		SIOseek(io, Entries[i], IL_SEEK_SET);
		PcxGetHeader(io, &Header);
		if (!PcxCheckHeader(&Header)) {
			iSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}

		Image = PcxDecode(io, &Header);
		if (Image == NULL)
			return IL_FALSE;

		if (i == 0) {
 			iTexImage(TargetImage, Image->Width, Image->Height, 1, Image->Bpp, Image->Format, Image->Type, Image->Data);
			Prev = TargetImage;
			Prev->Origin = IL_ORIGIN_UPPER_LEFT;
			iCloseImage(Image);
		}
		else {
			Prev->Next = Image;
			Prev = Prev->Next;
		}
	}

	return IL_TRUE;
}

// Internal function used to save the .dcx.
static ILboolean
iSaveDcxInternal(ILimage *Image)
{
	ILuint  EntryCount = 1, i, Start;
  SIO *   io;
  ILimage *TempImage;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	Start = SIOtell(io);

	TempImage = Image;
	while(TempImage->Next) {
		EntryCount ++;
		TempImage = TempImage->Next;
	}
	
	if (EntryCount > 1024) 
		EntryCount = 1024;

	SaveLittleUInt(io, 987654321);
	for (i=0; i<1024; i++) {
		SaveLittleUInt(io, 0);
	}

	i = 0;
	while(Image && i < 1024) {
		ILuint Pos = SIOtell(io);
		ILuint RelPos = Pos - Start;

		SIOseek(io, (i+1)*4 + Start, IL_SEEK_SET);
		SIOwrite(io, &RelPos, 1, 4);
		SIOseek(io, Pos, IL_SEEK_SET);

		if (!PcxEncode(io, Image))
			return IL_FALSE;

		Image = Image->Next;
	}

	return IL_TRUE;
}

static ILconst_string iFormatExtsDCX[] = { 
	IL_TEXT("dcx"), 
	NULL 
};

ILformat iFormatDCX = { 
	/* .Validate = */ iIsValidDcx, 
	/* .Load     = */ iLoadDcxInternal, 
	/* .Save     = */ iSaveDcxInternal, 
	/* .Exts     = */ iFormatExtsDCX
};

#endif//IL_NO_DCX
