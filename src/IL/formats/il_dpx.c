//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 2014-05-22 by Björn Paetzel
//
// Filename: src/IL/formats/il_dpx.c
//
// Description: Reads from a Digital Picture Exchange (.dpx) file.
//				Specifications for this format were	found at
//				http://www.cineon.com/ff_draft.php and
//				http://www.fileformat.info/format/dpx/.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_DPX
#include "il_dpx.h"

static ILboolean 
DpxGetFileInfo(SIO *io, DPX_FILE_INFO *FileInfo) {
	if (SIOread(io, FileInfo, 1, sizeof(*FileInfo)) != sizeof(*FileInfo))
		return IL_FALSE;

	if (FileInfo->Magic[0] == 'S') {
		BigUInt(&FileInfo->Offset);
		BigUInt(&FileInfo->FileSize);
		BigUInt(&FileInfo->DittoKey);
		BigUInt(&FileInfo->GenHdrSize);
		BigUInt(&FileInfo->IndHdrSize);
		BigUInt(&FileInfo->UserDataSize);
		BigUInt(&FileInfo->Key);
	} else {
		UInt(&FileInfo->Offset);
		UInt(&FileInfo->FileSize);
		UInt(&FileInfo->DittoKey);
		UInt(&FileInfo->GenHdrSize);
		UInt(&FileInfo->IndHdrSize);
		UInt(&FileInfo->UserDataSize);
		UInt(&FileInfo->Key);
	}

	return IL_TRUE;
}

static void
DpxFixImageElement(DPX_IMAGE_ELEMENT *ImageElement, ILboolean big) {
	if (big) {
		BigUInt  (&ImageElement->DataSign);
		BigUInt  (&ImageElement->RefLowData);
		BigUInt  (&ImageElement->RefHighData);
		BigUShort(&ImageElement->Packing);
		BigUShort(&ImageElement->Encoding);
		BigUInt  (&ImageElement->DataOffset);
		BigUInt  (&ImageElement->EolPadding);
		BigUInt  (&ImageElement->EoImagePadding);
	} else {
		UInt  (&ImageElement->DataSign);
		UInt  (&ImageElement->RefLowData);
		UInt  (&ImageElement->RefHighData);
		UShort(&ImageElement->Packing);
		UShort(&ImageElement->Encoding);
		UInt  (&ImageElement->DataOffset);
		UInt  (&ImageElement->EolPadding);
		UInt  (&ImageElement->EoImagePadding);
	}
}

static ILboolean 
DpxGetImageInfo(SIO *io, DPX_IMAGE_INFO *ImageInfo, ILboolean big) {
	if (SIOread(io, ImageInfo, 1, sizeof(*ImageInfo)) != sizeof(*ImageInfo))
		return IL_FALSE;

	if (big) {
		BigUShort(&ImageInfo->Orientation);
		BigUShort(&ImageInfo->NumElements);
		BigUInt  (&ImageInfo->Width);
		BigUInt  (&ImageInfo->Height);
	} else {
		UShort(&ImageInfo->Orientation);
		UShort(&ImageInfo->NumElements);
		UInt  (&ImageInfo->Width);
		UInt  (&ImageInfo->Height);
	}

	ILuint i;
	for (i = 0; i < 8; i++) {
		DpxFixImageElement(&ImageInfo->ImageElement[i], big);
	}

	return IL_TRUE;
}

static ILboolean
DpxGetImageOrient(SIO *io, DPX_IMAGE_ORIENT *ImageOrient, ILboolean big) {
	if (SIOread(io, ImageOrient, 1, sizeof(*ImageOrient)) != sizeof(*ImageOrient))
		return IL_FALSE;

	if (big) {
		BigUInt  	(&ImageOrient->XOffset);
		BigUInt  	(&ImageOrient->YOffset);
		BigUInt  	(&ImageOrient->XOrigSize);
		BigUInt  	(&ImageOrient->YOrigSize);
		BigUShort	(&ImageOrient->Border[0]);
		BigUShort	(&ImageOrient->Border[1]);
		BigUShort	(&ImageOrient->Border[2]);
		BigUShort	(&ImageOrient->Border[3]);
		BigUInt		(&ImageOrient->PixelAspect[0]);
		BigUInt		(&ImageOrient->PixelAspect[1]);
	} else {
		UInt  	(&ImageOrient->XOffset);
		UInt  	(&ImageOrient->YOffset);
		UInt  	(&ImageOrient->XOrigSize);
		UInt  	(&ImageOrient->YOrigSize);
		UShort	(&ImageOrient->Border[0]);
		UShort	(&ImageOrient->Border[1]);
		UShort	(&ImageOrient->Border[2]);
		UShort	(&ImageOrient->Border[3]);
		UInt		(&ImageOrient->PixelAspect[0]);
		UInt		(&ImageOrient->PixelAspect[1]);
	}

	return IL_TRUE;
}

// Internal function used to load the DPX.
static ILboolean iLoadDpxInternal(ILimage* image) {
	DPX_FILE_INFO			FileInfo;
	DPX_IMAGE_INFO		ImageInfo;
	DPX_IMAGE_ORIENT	ImageOrient;
	ILuint		i, NumElements, CurElem = 0;
	ILushort	Val, *ShortData;
	ILubyte		Data[8];
	ILenum		Format = 0;
	ILubyte		NumChans = 0;
	ILboolean bigEndian = IL_FALSE;

	if (image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SIO *io = &image->io;
	
	if (!DpxGetFileInfo(io, &FileInfo))
		return IL_FALSE;

	bigEndian = FileInfo.Magic[0] == 'S'; // "SDPX" -> big endian

	if (!DpxGetImageInfo(io, &ImageInfo, bigEndian))
		return IL_FALSE;

	if (!DpxGetImageOrient(io, &ImageOrient, bigEndian))
		return IL_FALSE;

	SIOseek(io, ImageInfo.ImageElement[CurElem].DataOffset, IL_SEEK_SET);

//@TODO: Deal with different origins!

	switch (ImageInfo.ImageElement[CurElem].Descriptor)
	{
		case 6:  // Luminance data
			Format = IL_LUMINANCE;
			NumChans = 1;
			break;
		case 50:  // RGB data
			Format = IL_RGB;
			NumChans = 3;
			break;
		case 51:  // RGBA data
			Format = IL_RGBA;
			NumChans = 4;
			break;
		default:
			ilSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	// These are all on nice word boundaries.
	switch (ImageInfo.ImageElement[CurElem].BitSize)
	{
		case 8:
		case 16:
		case 32:
			if (!ilTexImage(ImageInfo.Width, ImageInfo.Height, 1, NumChans, Format, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			image->Origin = IL_ORIGIN_UPPER_LEFT;
			if (iCurImage->io.read(iCurImage->io.handle, image->Data, image->SizeOfData, 1) != 1)
				return IL_FALSE;
			goto finish;
	}

	// The rest of these do not end on word boundaries.
	if (ImageInfo.ImageElement[CurElem].Packing == 1) {
		// Here we have it padded out to a word boundary, so the extra bits are filler.
		switch (ImageInfo.ImageElement[CurElem].BitSize)
		{
			case 10:
				//@TODO: Support other formats!
				/*if (Format != IL_RGB) {
					ilSetError(IL_FORMAT_NOT_SUPPORTED);
					return IL_FALSE;
				}*/
				switch (Format)
				{
					case IL_LUMINANCE:
						if (!ilTexImage(ImageInfo.Width, ImageInfo.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						image->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)image->Data;
						NumElements = image->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							iCurImage->io.read(iCurImage->io.handle, Data, 1, 2);
							Val = ((Data[0] << 2) + ((Data[1] & 0xC0) >> 6)) << 6;  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
						}
						break;

					case IL_RGB:
						if (!ilTexImage(ImageInfo.Width, ImageInfo.Height, 1, 3, IL_RGB, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						image->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)image->Data;
						NumElements = image->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							iCurImage->io.read(iCurImage->io.handle, Data, 1, 4);
							Val = ((Data[0] << 2) + ((Data[1] & 0xC0) >> 6)) << 6;  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
							Val = (((Data[1] & 0x3F) << 4) + ((Data[2] & 0xF0) >> 4)) << 6;  // Use the next 10 bits.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Same fill
							Val = (((Data[2] & 0x0F) << 6) + ((Data[3] & 0xFC) >> 2)) << 6;  // And finally use the last 10 bits (ignores the last 2 bits).
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Same fill
						}
						break;

					case IL_RGBA:  // Is this even a possibility?  There is a ton of wasted space here!
						if (!ilTexImage(ImageInfo.Width, ImageInfo.Height, 1, 4, IL_RGBA, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						image->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)image->Data;
						NumElements = image->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							iCurImage->io.read(iCurImage->io.handle, Data, 1, 8);
							Val = (Data[0] << 2) + ((Data[1] & 0xC0) >> 6);  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
							Val = ((Data[1] & 0x3F) << 4) + ((Data[2] & 0xF0) >> 4);  // Use the next 10 bits.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Same fill
							Val = ((Data[2] & 0x0F) << 6) + ((Data[3] & 0xFC) >> 2);  // Use the next 10 bits.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Same fill
							Val = ((Data[3] & 0x03) << 8) + Data[4];  // And finally use the last 10 relevant bits (skips 3 whole bytes worth of padding!).
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Last fill
						}
						break;
				}
				break;

			//case 1:
			//case 12:
			default:
				ilSetError(IL_FORMAT_NOT_SUPPORTED);
				return IL_FALSE;
		}
	}
	else if (ImageInfo.ImageElement[0].Packing == 0) {
		// Here we have the data packed so that it is never aligned on a word boundary.
		/*File = bfile(iGetFile());
		if (File == NULL)
			return IL_FALSE;  //@TODO: Error?
		ShortData = (ILushort*)image->Data;
		NumElements = image->SizeOfData / 2;
		for (i = 0; i < NumElements; i++) {
			//bread(&Val, 1, 10, File);
			Val = breadVal(10, File);
			ShortData[i] = (Val << 6) | (Val >> 4);
		}
		bclose(File);*/

		ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
	else {
		ilSetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;  //@TODO: Take care of this in an iCheckDpx* function.
	}

finish:
	return ilFixImage();
}

static ILboolean 
iIsValidDpx(SIO *io) {
	ILuint magic;

	if (SIOread(io, &magic, 1, sizeof(magic)) != sizeof(magic))
		return IL_FALSE;

	return magic == 0x53445058 || magic == 0x58504453;
}

ILconst_string iFormatExtsDPX[] = { 
	"dpx",
	NULL 
};

ILformat iFormatDPX = { 
	.Validate = iIsValidDpx, 
	.Load     = iLoadDpxInternal, 
	.Save     = NULL, 
	.Exts     = iFormatExtsDPX
};

#endif//IL_NO_DPX

