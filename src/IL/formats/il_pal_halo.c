//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_pal.c
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_pal.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

static ILboolean iIsValidHaloPal(SIO *io);
static ILboolean iLoadHaloPal(ILimage *Image);

#include "pack_push.h"
typedef struct HALOHEAD
{
	ILubyte		Id[2];  // 'AH'
	ILshort		Version;
	ILshort		Size;
	ILbyte		Filetype;
	ILbyte		Subtype;
	//ILshort	Brdid, Grmode;
	ILint			Ignored;
	ILushort	MaxIndex;  // Colors = maxindex + 1
	ILushort	MaxRed;
	ILushort	MaxGreen;
	ILushort	MaxBlue;
	/*ILbyte	Signature[8];
	ILbyte		Filler[12];*/
	ILbyte		Filler[20];  // Always 0 by PSP 4
} HALOHEAD;
#include "pack_pop.h"


ILboolean iIsValidHaloPal(SIO *io) {
	ILuint 		First = SIOtell(io);
	HALOHEAD  Head;
	ILuint    Read = SIOread(io, &Head, 1, sizeof(Head));

	SIOseek(io, First, IL_SEEK_SET);

	Short(&Head.Version);

	return Read == sizeof(Head) 
			&& !memcmp(Head.Id, "AH", 2)
			&& Head.Version == 0xe3; // TODO: check if this is sufficient
}

//! Loads a Halo formatted palette (.pal) file.
static ILboolean iLoadHaloPal(ILimage *Image) {
	HALOHEAD	HaloHead;
	ILushort	*TempPal;
	ILuint		i, Size;
	ILpal NewPal;
	SIO *io = &Image->io;
	ILuint BlockPos;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &Image->io;

	if (SIOread(io, &HaloHead, sizeof(HALOHEAD), 1) != 1)
		return IL_FALSE;

	Short(&HaloHead.Version);
	Short(&HaloHead.Size);
	Short(&HaloHead.MaxIndex);  // Colors = maxindex + 1
	UShort(&HaloHead.MaxRed);
	UShort(&HaloHead.MaxGreen);
	UShort(&HaloHead.MaxBlue);

	if ( HaloHead.Id[0] != 'A' || HaloHead.Id[1] != 'H' 
		|| HaloHead.Version != 0xe3 ) {
		iSetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	Size = (HaloHead.MaxIndex + 1) * 3;
	TempPal = (ILushort*)ialloc(Size * sizeof(ILushort));
	if (TempPal == NULL) {
		return IL_FALSE;
	}

	BlockPos = SIOtell(io) % 512;

	for (i = 0; i < Size / 3; i++) {
		ILuint AfterPos = BlockPos + 3 * sizeof(ILushort);
		if (AfterPos > 512) {
			// tuple would cross 512 byte boundary, skip to beginning of next block
			SIOseek(io, 512 - SIOtell(io)%512, IL_SEEK_CUR);
			BlockPos = 0;
		}

		if (SIOread(io, TempPal + 3 * i, sizeof(ILushort), 3) != 3) {
			ifree(TempPal);
			return IL_FALSE;
		}
		BlockPos += 3 * sizeof(ILushort);
	}

	imemclear(&NewPal, sizeof(NewPal));

	NewPal.PalType = IL_PAL_RGB24;
	NewPal.PalSize = Size;
	NewPal.Palette = (ILubyte*)ialloc(NewPal.PalSize);

	if (NewPal.Palette == NULL) {
		return IL_FALSE;
	}

	for (i = 0; i < NewPal.PalSize/3; i++) {
		NewPal.Palette[i*3+0] = (ILubyte)( (255 * TempPal[i*3+0]) / HaloHead.MaxRed );
		NewPal.Palette[i*3+1] = (ILubyte)( (255 * TempPal[i*3+1]) / HaloHead.MaxGreen );
		NewPal.Palette[i*3+2] = (ILubyte)( (255 * TempPal[i*3+2]) / HaloHead.MaxBlue );
	}
	ifree(TempPal);

	if ( Image->Pal.Palette 
		&& Image->Pal.PalSize > 0 
		&& Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
	}
	Image->Pal = NewPal;

	return IL_TRUE;
}

static ILconst_string iFormatExtsHALO_PAL[] = { 
	IL_TEXT("pal"),
  NULL 
};

ILformat iFormatHALO_PAL = { 
  /* .Validate = */ iIsValidHaloPal, 
  /* .Load     = */ iLoadHaloPal, 
  /* .Save     = */ NULL,
  /* .Exts     = */ iFormatExtsHALO_PAL
};
