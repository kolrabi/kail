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
	ILubyte   Read = SIOread(io, &Head, 1, sizeof(Head));

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

	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &Image->io;

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
		ilSetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	Size = (HaloHead.MaxIndex + 1) * 3;
	TempPal = (ILushort*)ialloc(Size * sizeof(ILushort));
	if (TempPal == NULL) {
		return IL_FALSE;
	}

	if (SIOread(io, TempPal, sizeof(ILushort), Size) != Size) {
		ifree(TempPal);
		return IL_FALSE;
	}

	if (Image->Pal.Palette && Image->Pal.PalSize > 0 && Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
	}
	Image->Pal.PalType = IL_PAL_RGB24;
	Image->Pal.PalSize = Size;
	Image->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);

	if (Image->Pal.Palette == NULL) {
		return IL_FALSE;
	}

	for (i = 0; i < Image->Pal.PalSize; i++, TempPal++) {
		Image->Pal.Palette[i] = (ILubyte)*TempPal;
	}
	TempPal -= Image->Pal.PalSize;
	ifree(TempPal);

	return IL_TRUE;
}

ILconst_string iFormatExtsHALO_PAL[] = { 
	"pal",
  NULL 
};

ILformat iFormatHALO_PAL = { 
  .Validate = iIsValidHaloPal, 
  .Load     = iLoadHaloPal, 
  .Save     = NULL,
  .Exts     = iFormatExtsHALO_PAL
};
