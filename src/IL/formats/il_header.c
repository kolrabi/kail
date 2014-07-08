//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Last modified: 10/10/2006
//
// Filename: src-IL/src/il_header.c
//
// Description: Generates a C-style header file for the current image.
//
//-----------------------------------------------------------------------------

#ifndef IL_NO_CHEAD

#include "il_internal.h"

// Just a guess...let's see what's purty!
#define MAX_LINE_WIDTH 14

//! Generates a C-style header file for the current image.
static ILboolean 
iSaveCHEADInternal(ILimage* image)
{
	const char *InternalName = "IL_IMAGE";
	// FILE		*HeadFile;
	ILuint		i = 0, j;
	ILimage		*TempImage;
	const char	*Name;
	char        tmp[512];
	SIO *       io;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;

	Name = iGetString(image, IL_META_DOCUMENT_NAME);
	if (Name == NULL)
		Name = InternalName;

	if (image->Bpc > 1) {
		TempImage = iConvertImage(image, image->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
           return IL_FALSE;
	} else {
		TempImage = image;
	}

	SIOputs(io, "//#include <il/il.h>\n");
	SIOputs(io, "// C Image Header:\n\n\n");
	SIOputs(io, "// IMAGE_BPP is in bytes per pixel, *not* bits\n");

	snprintf(tmp, sizeof(tmp), "#define IMAGE_BPP     %d\n",image->Bpp);
	SIOputs(io, tmp);

	snprintf(tmp, sizeof(tmp), "#define IMAGE_WIDTH   %d\n", image->Width);
	SIOputs(io, tmp);
	
	snprintf(tmp, sizeof(tmp), "#define IMAGE_HEIGHT  %d\n", image->Height);	
	SIOputs(io, tmp);
	
	snprintf(tmp, sizeof(tmp), "#define IMAGE_DEPTH   %d\n\n\n", image->Depth);
	SIOputs(io, tmp);
	
	snprintf(tmp, sizeof(tmp), "#define IMAGE_TYPE    0x%X\n", image->Type);
	SIOputs(io, tmp);
	
	snprintf(tmp, sizeof(tmp), "#define IMAGE_FORMAT  0x%X\n\n\n", image->Format);
	SIOputs(io, tmp);

	snprintf(tmp, sizeof(tmp), "ILubyte %s[] = {\n", Name);
	SIOputs(io, tmp);
        
	for (; i < TempImage->SizeOfData; i += MAX_LINE_WIDTH) {
		SIOputs(io, "\t");
		for (j = 0; j < MAX_LINE_WIDTH; j++) {
			if (i + j >= TempImage->SizeOfData - 1) {
				snprintf(tmp, sizeof(tmp), " %4d", TempImage->Data[i+j]);
				SIOputs(io, tmp);
				break;
			}	else {
				snprintf(tmp, sizeof(tmp), " %4d,", TempImage->Data[i+j]);
				SIOputs(io, tmp);
			}
		}
		SIOputs(io, "\n");
	}

	if (TempImage != image)
		iCloseImage(TempImage);

	SIOputs(io, "};\n");

	if (image->Pal.Palette && image->Pal.PalSize && image->Pal.PalType != IL_PAL_NONE) {
		SIOputs(io, "\n\n");

		snprintf(tmp, sizeof(tmp), "#define IMAGE_PALSIZE %u\n\n", image->Pal.PalSize);
		SIOputs(io, tmp);

		snprintf(tmp, sizeof(tmp), "#define IMAGE_PALTYPE 0x%X\n\n", image->Pal.PalType);
		SIOputs(io, tmp);

		snprintf(tmp, sizeof(tmp), "ILubyte %sPal[] = {\n", Name);
		SIOputs(io, tmp);

		for (i = 0; i < image->Pal.PalSize; i += MAX_LINE_WIDTH) {
			SIOputs(io, "\t");
			for (j = 0; j < MAX_LINE_WIDTH; j++) {
				if (i + j >= image->Pal.PalSize - 1) {
					snprintf(tmp, sizeof(tmp), " %4d", image->Pal.Palette[i+j]);
					SIOputs(io, tmp);
					break;
				} else {
					snprintf(tmp, sizeof(tmp), " %4d,", image->Pal.Palette[i+j]);
					SIOputs(io, tmp);
				}
			}
			SIOputs(io, "\n");
		}

		SIOputs(io, "};\n");
	}

	return IL_TRUE;
}

ILconst_string iFormatExtsCHEAD[] = { 
	IL_TEXT("h"), 
	NULL 
};

ILformat iFormatCHEAD = { 
	/* .Validate = */ NULL, 
	/* .Load     = */ NULL, 
	/* .Save     = */ iSaveCHEADInternal,
	/* .Exts     = */ iFormatExtsCHEAD
};

#endif//IL_NO_CHEAD
