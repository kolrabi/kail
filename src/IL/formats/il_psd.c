//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_psd.c
//
// Description: Reads and writes Photoshop (.psd) files.
//
//-----------------------------------------------------------------------------


// Information about the .psd format was taken from Adobe's PhotoShop SDK at
// http://partners.adobe.com/asn/developer/gapsdk/PhotoshopSDK.html
// Information about the Packbits compression scheme was found at
//	http://partners.adobe.com/asn/developer/PDFS/TN/TIFF6.pdf

#include "il_internal.h"
#ifndef IL_NO_PSD
#include "il_psd.h"


ILushort	ChannelNum;

static ILboolean	iCheckPsd (PSDHEAD *Header);
static ILboolean	ReadPsd 	(ILimage* image, PSDHEAD *Head);
static ILboolean	ReadGrey(ILimage* image, PSDHEAD *Head);
static ILboolean	ReadIndexed(ILimage* image, PSDHEAD *Head);
static ILboolean	ReadRGB(ILimage* image, PSDHEAD *Head);
static ILboolean	ReadCMYK(ILimage* image, PSDHEAD *Head);
static ILuint		* GetCompChanLen(SIO*io, PSDHEAD *Head);

// static ILboolean	iLoadPsdInternal(ILimage *image);
ILboolean	PsdGetData(ILimage* image, PSDHEAD *Head, void *Buffer, ILboolean Compressed);
ILboolean	ParseResources(ILimage* image, ILuint ResourceSize, ILubyte *Resources);
ILboolean	GetSingleChannel(ILimage* image, PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed);
ILboolean	iSavePsdInternal(ILimage* image);

// TODO: share these
static float ubyte_to_float(ILubyte val)
{
	return ((float)val) / 255.0f;
}
static float ushort_to_float(ILushort val)
{
	return ((float)val) / 65535.0f;
}

static ILubyte float_to_ubyte(float val)
{
	return (ILubyte)(val * 255.0f);
}
static ILushort float_to_ushort(float val)
{
	return (ILushort)(val * 65535.0f);
}


// Internal function used to get the Psd header from the current file.
static ILboolean iGetPsdHead(SIO *io, PSDHEAD *Header)
{
	if (SIOread(io, Header, sizeof(*Header), 1) != 1)
		return IL_FALSE;

	BigUShort(&Header->Version);
	BigUShort(&Header->Channels);

	BigUInt  (&Header->Height);
	BigUInt  (&Header->Width);
	BigUShort(&Header->Depth);
	BigUShort(&Header->Mode);

	return IL_TRUE;
}

// Internal function to get the header and check it.
static ILboolean iIsValidPsd(SIO *io) {
	PSDHEAD		Head;
	ILuint  	Start     = SIOtell(io);
	ILboolean GotHeader = iGetPsdHead(io, &Head);
	SIOseek(io, Start, IL_SEEK_SET);

	return GotHeader && iCheckPsd(&Head);
}

// Internal function used to check if the HEADER is a valid Psd header.
static ILboolean iCheckPsd(PSDHEAD *Header) {
	ILuint i;

	if (memcmp(Header->Signature, "8BPS", 4))
		return IL_FALSE;

	if (Header->Version != 1)
		return IL_FALSE;

	for (i = 0; i < 6; i++) {
		if (Header->Reserved[i] != 0)
			return IL_FALSE;
	}

	if (Header->Channels < 1 || Header->Channels > 24)
		return IL_FALSE;

	if (Header->Height < 1 || Header->Width < 1)
		return IL_FALSE;

	if (Header->Depth != 1 && Header->Depth != 8 && Header->Depth != 16)
		return IL_FALSE;

	return IL_TRUE;
}

// Internal function used to load the Psd.
static ILboolean iLoadPsdInternal(ILimage* image) {
	PSDHEAD	Header;
	SIO *io;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;

	if (!iGetPsdHead(io, &Header) || !iCheckPsd(&Header)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadPsd(image, &Header))
		return IL_FALSE;

	image->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}

static ILboolean ReadPsd(ILimage* image, PSDHEAD *Head) {
	switch (Head->Mode) {
		case 1:  // Greyscale
			return ReadGrey(image, Head);
		case 2:  // Indexed
			return ReadIndexed(image, Head);
		case 3:  // RGB
			return ReadRGB(image, Head);
		case 4:  // CMYK
			return ReadCMYK(image, Head);
	}

	iSetError(IL_FORMAT_NOT_SUPPORTED);
	return IL_FALSE;
}

static ILboolean ReadGrey(ILimage* image, PSDHEAD *Head) {
	ILuint		ColorMode, ResourceSize, MiscInfo;
	ILushort	Compressed;
	ILenum		Type;
	ILubyte * Resources = NULL;
	SIO *     io = &image->io; 

	ColorMode = GetBigUInt(io);  // Skip over the 'color mode data section'
	SIOseek(io, ColorMode, IL_SEEK_CUR);

	ResourceSize 	= GetBigUInt(io);  // Read the 'image resources section'
	Resources 		= (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}

	if (SIOread(io, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(io);
	SIOseek(io, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(io);

	ChannelNum = Head->Channels;
	Head->Channels = 1;  // Temporary to read only one channel...some greyscale .psd files have 2.

	switch (Head->Depth) {
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			iSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!iTexImage(image, Head->Width, Head->Height, 1, 1, IL_LUMINANCE, Type, NULL))
		goto cleanup_error;

	if (!PsdGetData(image, Head, image->Data, (ILboolean)Compressed))
		goto cleanup_error;

	if (!ParseResources(image, ResourceSize, Resources))
		goto cleanup_error;

	ifree(Resources);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	return IL_FALSE;
}

static ILboolean ReadIndexed(ILimage* image, PSDHEAD *Head) {
	ILuint		ColorMode, ResourceSize, MiscInfo, i, j, NumEnt;
	ILushort	Compressed;
	ILubyte * Palette = NULL;
	ILubyte * Resources = NULL;
	SIO *     io = &image->io; 

	ColorMode = GetBigUInt(io);  // Skip over the 'color mode data section'
	if (ColorMode % 3 != 0) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	Palette = (ILubyte*)ialloc(ColorMode);
	if (Palette == NULL)
		return IL_FALSE;

	if (SIOread(io, Palette, 1, ColorMode) != ColorMode)
		goto cleanup_error;

	ResourceSize = GetBigUInt(io);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}

	if (SIOread(io, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(io);
	if (SIOeof(io))
		goto cleanup_error;

	SIOseek(io, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(io);
	if (SIOeof(io))
		goto cleanup_error;

	if (Head->Channels != 1 || Head->Depth != 8) {
		iSetError(IL_FORMAT_NOT_SUPPORTED);
		goto cleanup_error;
	}

	ChannelNum = Head->Channels;

	if (!iTexImage(image, Head->Width, Head->Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		goto cleanup_error;

	image->Pal.Palette = (ILubyte*)ialloc(ColorMode);
	if (image->Pal.Palette == NULL) {
		goto cleanup_error;
	}
	image->Pal.PalSize = ColorMode;
	image->Pal.PalType = IL_PAL_RGB24;

	NumEnt = image->Pal.PalSize / 3;
	for (i = 0, j = 0; i < image->Pal.PalSize; i += 3, j++) {
		image->Pal.Palette[i  ] = Palette[j];
		image->Pal.Palette[i+1] = Palette[j+NumEnt];
		image->Pal.Palette[i+2] = Palette[j+NumEnt*2];
	}
	ifree(Palette);
	Palette = NULL;

	if (!PsdGetData(image, Head, image->Data, (ILboolean)Compressed))
		goto cleanup_error;

	ParseResources(image, ResourceSize, Resources);
	ifree(Resources);
	Resources = NULL;

	return IL_TRUE;

cleanup_error:
	ifree(Palette);
	ifree(Resources);

	return IL_FALSE;
}


static ILboolean ReadRGB(ILimage* image, PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo;
	ILushort	Compressed;
	ILenum		Format, Type;
	ILubyte * Resources = NULL;
	SIO *  		io = &image->io;

	ColorMode = GetBigUInt(io);  // Skip over the 'color mode data section'
	SIOseek(io, ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt(io);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL)
		return IL_FALSE;

	if (SIOread(io, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(io);;
	SIOseek(io, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(io);

	ChannelNum = Head->Channels;
	if (Head->Channels == 3) {
		Format = IL_RGB;
	} else if (Head->Channels == 4) {
		Format = IL_RGBA;
	}	else if (Head->Channels >= 5)	{
		// Additional channels are accumulated as a single alpha channel, since
		// if an image does not have a layer set as the "background", but also
		// has a real alpha channel, there will be 5 channels (or more).
		Format = IL_RGBA;
	}	else {
		iSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	switch (Head->Depth) {
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			iSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!iTexImage(image, Head->Width, Head->Height, 1, (Format==IL_RGB) ? 3 : 4, Format, Type, NULL))
		goto cleanup_error;

	if (!PsdGetData(image, Head, image->Data, (ILboolean)Compressed))
		goto cleanup_error;

	if (!ParseResources(image, ResourceSize, Resources))
		goto cleanup_error;

	ifree(Resources);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	return IL_FALSE;
}


ILboolean ReadCMYK(ILimage* image, PSDHEAD *Head) {
	ILuint		ColorMode, ResourceSize, MiscInfo, Size, i, j;
	ILushort	Compressed;
	ILenum		Format, Type;
	ILubyte * Resources = NULL;
	ILubyte * KChannel = NULL;
	SIO *  		io = &image->io;

	ColorMode = GetBigUInt(io);;  // Skip over the 'color mode data section'
	SIOseek(io, ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt(io);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}

	if (SIOread(io, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(io);
	SIOseek(io, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(io);

	switch (Head->Channels)
	{
		case 4:
			Format = IL_RGB;
			ChannelNum = 4;
			Head->Channels = 3;
			break;
		case 5:
			Format = IL_RGBA;
			ChannelNum = 5;
			Head->Channels = 4;
			break;
		default:
			iSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			iSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!iTexImage(image, Head->Width, Head->Height, 1, (ILubyte)Head->Channels, Format, Type, NULL))
		goto cleanup_error;

	if (!PsdGetData(image, Head, image->Data, (ILboolean)Compressed))
		goto cleanup_error;

	Size = image->Bpc * image->Width * image->Height;
	KChannel = (ILubyte*)ialloc(Size);
	if (KChannel == NULL)
		goto cleanup_error;

	if (!GetSingleChannel(image, Head, KChannel, (ILboolean)Compressed))
		goto cleanup_error;

	if (Format == IL_RGB) {
		for (i = 0, j = 0; i < image->SizeOfData; i += 3, j++) {
			image->Data[i  ] = (image->Data[i  ] * KChannel[j]) >> 8;
			image->Data[i+1] = (image->Data[i+1] * KChannel[j]) >> 8;
			image->Data[i+2] = (image->Data[i+2] * KChannel[j]) >> 8;
		}
	}
	else {  // IL_RGBA
		// The KChannel array really holds the alpha channel on this one.
		for (i = 0, j = 0; i < image->SizeOfData; i += 4, j++) {
			image->Data[i  ] = (image->Data[i  ] * image->Data[i+3]) >> 8;
			image->Data[i+1] = (image->Data[i+1] * image->Data[i+3]) >> 8;
			image->Data[i+2] = (image->Data[i+2] * image->Data[i+3]) >> 8;
			image->Data[i+3] = KChannel[j];  // Swap 'K' with alpha channel.
		}
	}

	if (!ParseResources(image, ResourceSize, Resources))
		goto cleanup_error;

	ifree(Resources);
	ifree(KChannel);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	ifree(KChannel);
	return IL_FALSE;
}

static ILuint *GetCompChanLen(SIO *io, PSDHEAD *Head) {
	ILushort	*RleTable;
	ILuint		*ChanLen, c, i, j;

	RleTable = (ILushort*)ialloc(Head->Height * ChannelNum * sizeof(ILushort));
	ChanLen = (ILuint*)ialloc(ChannelNum * sizeof(ILuint));
	if (RleTable == NULL || ChanLen == NULL) {
		return NULL;
	}

	if (SIOread(io, RleTable, sizeof(ILushort), Head->Height * ChannelNum) != Head->Height * ChannelNum) {
		ifree(RleTable);
		ifree(ChanLen);
		return NULL;
	}
#ifdef WORDS_LITTLEENDIAN
	for (i = 0; i < Head->Height * ChannelNum; i++) {
		iSwapUShort(&RleTable[i]);
	}
#endif

	imemclear(ChanLen, ChannelNum * sizeof(ILuint));
	for (c = 0; c < ChannelNum; c++) {
		j = c * Head->Height;
		for (i = 0; i < Head->Height; i++) {
			ChanLen[c] += RleTable[i + j];
		}
	}

	ifree(RleTable);

	return ChanLen;
}



static const ILuint READ_COMPRESSED_SUCCESS					= 0;
static const ILuint READ_COMPRESSED_ERROR_FILE_CORRUPT		= 1;
static const ILuint READ_COMPRESSED_ERROR_FILE_READ_ERROR	= 2;

static ILuint ReadCompressedChannel(SIO *io, const ILuint ChanLen, ILuint Size, ILubyte* Channel)
{
	ILuint		i;
	ILint		Run;
	ILbyte		HeadByte;

	(void)ChanLen;

	/*
	if (iGetHint(IL_MEM_SPEED_HINT) == IL_FASTEST)
		PreCache = IL_TRUE;
	*/

	for (i = 0; i < Size; ) {
		HeadByte = SIOgetc(io);

		if (HeadByte >= 0) {  //  && HeadByte <= 127
			if (i + HeadByte > Size)
			{
				return READ_COMPRESSED_ERROR_FILE_CORRUPT;
			}
			if (SIOread(io, Channel + i, HeadByte + 1, 1) != 1)
			{
				return READ_COMPRESSED_ERROR_FILE_READ_ERROR;
			}

			i += HeadByte + 1;
		}
		if (HeadByte >= -127 && HeadByte <= -1) {
			Run = SIOgetc(io);
			if (Run == IL_EOF)
			{
				return READ_COMPRESSED_ERROR_FILE_READ_ERROR;
			}
			if (i + (-HeadByte + 1) > Size)
			{
				return READ_COMPRESSED_ERROR_FILE_CORRUPT;
			}

			memset(Channel + i, Run, -HeadByte + 1);
			i += -HeadByte + 1;
		}
		if (HeadByte == -128)
		{ }  // Noop
	}

	return READ_COMPRESSED_SUCCESS;
}


ILboolean PsdGetData(ILimage* image, PSDHEAD *Head, void *Buffer, ILboolean Compressed)
{
	ILuint		c, x, y, i, Size, ReadResult, NumChan;
	ILubyte		*Channel = NULL;
	ILushort	*ShortPtr;
	ILuint		*ChanLen = NULL;

	SIO *     io = &image->io;

	(void)Buffer;

	// Added 01-07-2009: This is needed to correctly load greyscale and
	//  paletted images.
	switch (Head->Mode)
	{
		case 1:
		case 2:
			NumChan = 1;
			break;
		default:
			NumChan = 3;
	}

	Channel = (ILubyte*)ialloc(Head->Width * Head->Height * image->Bpc);
	if (Channel == NULL) {
		return IL_FALSE;
	}
	ShortPtr = (ILushort*)Channel;

	// @TODO: Add support for this in, though I have yet to run across a .psd
	//	file that uses this.
	if (Compressed && image->Type == IL_UNSIGNED_SHORT) {
		iSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	if (!Compressed) {
		if (image->Bpc == 1) {
			for (c = 0; c < NumChan; c++) {
				i = 0;
				if (SIOread(io, Channel, Head->Width * Head->Height, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
					for (x = 0; x < image->Bps; x += image->Bpp, i++) {
						image->Data[y + x + c] = Channel[i];
					}
				}
			}
			// Accumulate any remaining channels into a single alpha channel
			//@TODO: This needs to be changed for greyscale images.
			for (; c < Head->Channels; c++) {
				i = 0;
				if (SIOread(io, Channel, Head->Width * Head->Height, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
					for (x = 0; x < image->Bps; x += image->Bpp, i++) {
						float curVal = ubyte_to_float(image->Data[y + x + 3]);
						float newVal = ubyte_to_float(Channel[i]);
						image->Data[y + x + 3] = float_to_ubyte(curVal * newVal);
					}
				}
			}
		}
		else {  // image->Bpc == 2
			for (c = 0; c < NumChan; c++) {
				i = 0;
				if (SIOread(io, Channel, Head->Width * Head->Height * 2, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				image->Bps /= 2;
				for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
					for (x = 0; x < image->Bps; x += image->Bpp, i++) {
					 #ifndef WORDS_BIGENDIAN
						iSwapUShort(ShortPtr+i);
					 #endif
						((ILushort*)image->Data)[y + x + c] = ShortPtr[i];
					}
				}
				image->Bps *= 2;
			}
			// Accumulate any remaining channels into a single alpha channel
			//@TODO: This needs to be changed for greyscale images.
			for (; c < Head->Channels; c++) {
				i = 0;
				if (SIOread(io, Channel, Head->Width * Head->Height * 2, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				image->Bps /= 2;
				for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
					for (x = 0; x < image->Bps; x += image->Bpp, i++) {
						float curVal = ushort_to_float(((ILushort*)image->Data)[y + x + 3]);
						float newVal = ushort_to_float(ShortPtr[i]);
						((ILushort*)image->Data)[y + x + 3] = float_to_ushort(curVal * newVal);
					}
				}
				image->Bps *= 2;
			}
		}
	}
	else {
		ChanLen = GetCompChanLen(io, Head);

		Size = Head->Width * Head->Height;
		for (c = 0; c < NumChan; c++) {
			ReadResult = ReadCompressedChannel(io, ChanLen[c], Size, Channel);
			if (ReadResult == READ_COMPRESSED_ERROR_FILE_CORRUPT)
				goto file_corrupt;
			else if (ReadResult == READ_COMPRESSED_ERROR_FILE_READ_ERROR)
				goto file_read_error;

			i = 0;
			for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
				for (x = 0; x < image->Bps; x += image->Bpp, i++) {
					image->Data[y + x + c] = Channel[i];
				}
			}
		}

		// Initialize the alpha channel to solid
		//@TODO: This needs to be changed for greyscale images.
		if (Head->Channels >= 4) {
			for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
				for (x = 0; x < image->Bps; x += image->Bpp) {
					image->Data[y + x + 3] = 255;
				}
			}
					
			for (; c < Head->Channels; c++) {
				ReadResult = ReadCompressedChannel(io, ChanLen[c], Size, Channel);
				if (ReadResult == READ_COMPRESSED_ERROR_FILE_CORRUPT)
					goto file_corrupt;
				else if (ReadResult == READ_COMPRESSED_ERROR_FILE_READ_ERROR)
					goto file_read_error;

				i = 0;
				for (y = 0; y < Head->Height * image->Bps; y += image->Bps) {
					for (x = 0; x < image->Bps; x += image->Bpp, i++) {
						float curVal = ubyte_to_float(image->Data[y + x + 3]);
						float newVal = ubyte_to_float(Channel[i]);
						image->Data[y + x + 3] = float_to_ubyte(curVal * newVal);
					}
				}
			}
		}

		ifree(ChanLen);
	}

	ifree(Channel);

	return IL_TRUE;

file_corrupt:
	ifree(ChanLen);
	ifree(Channel);
	iSetError(IL_ILLEGAL_FILE_VALUE);
	return IL_FALSE;

file_read_error:
	ifree(ChanLen);
	ifree(Channel);
	return IL_FALSE;
}


ILboolean ParseResources(ILimage* image, ILuint ResourceSize, ILubyte *Resources)
{
	ILushort	ID;
	ILubyte		NameLen;
	ILuint		Size;

	if (Resources == NULL) {
		iSetError(IL_INTERNAL_ERROR);
		return IL_FALSE;
	}

	while (ResourceSize > 13) {  // Absolutely has to be larger than this.
		if (strncmp("8BIM", (const char*)Resources, 4)) {
			//return IL_FALSE;
			return IL_TRUE;  // 05-30-2002: May not necessarily mean corrupt data...
		}
		Resources += 4;

		ID = *((ILushort*)Resources);
		BigUShort(&ID);
		Resources += 2;

		NameLen = *Resources++;
		// NameLen + the byte it occupies must be padded to an even number, so NameLen must be odd.
		NameLen = NameLen + (NameLen & 1 ? 0 : 1);
		Resources += NameLen;

		// Get the resource data size.
		Size = *((ILuint*)Resources);
		BigUInt(&Size);
		Resources += 4;

		ResourceSize -= (4 + 2 + 1 + NameLen + 4);

		switch (ID)
		{
			case 0x040F:  // ICC Profile
				if (Size > ResourceSize) {  // Check to make sure we are not going past the end of Resources.
					iSetError(IL_ILLEGAL_FILE_VALUE);
					return IL_FALSE;
				}
				image->Profile = (ILubyte*)ialloc(Size);
				if (image->Profile == NULL) {
					return IL_FALSE;
				}
				memcpy(image->Profile, Resources, Size);
				image->ProfileSize = Size;
				break;

			default:
				break;
		}

		if (Size & 1)  // Must be an even number.
			Size++;
		ResourceSize -= Size;
		Resources += Size;
	}

	return IL_TRUE;
}


ILboolean GetSingleChannel(ILimage* image, PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed)
{
	ILuint		i;
	// ILushort	*ShortPtr;
	ILbyte		HeadByte;
	ILint		Run;
	SIO *  		io = &image->io;

	// ShortPtr = (ILushort*)Buffer;

	if (!Compressed) {
		if (image->Bpc == 1) {
			if (SIOread(io, Buffer, Head->Width * Head->Height, 1) != 1)
				return IL_FALSE;
		}
		else {  // image->Bpc == 2
			if (SIOread(io, Buffer, Head->Width * Head->Height * 2, 1) != 1)
				return IL_FALSE;
		}
	}
	else {
		for (i = 0; i < Head->Width * Head->Height; ) {
			HeadByte = SIOgetc(io);

			if (HeadByte >= 0) {  //  && HeadByte <= 127
				if (SIOread(io, Buffer + i, HeadByte + 1, 1) != 1)
					return IL_FALSE;
				i += HeadByte + 1;
			}
			if (HeadByte >= -127 && HeadByte <= -1) {
				Run = SIOgetc(io);
				if (Run == IL_EOF)
					return IL_FALSE;
				memset(Buffer + i, Run, -HeadByte + 1);
				i += -HeadByte + 1;
			}
			if (HeadByte == -128)
			{ }  // Noop
		}
	}

	return IL_TRUE;
}


// Internal function used to save the Psd.
ILboolean iSavePsdInternal(ILimage* image)
{
	ILubyte		*Signature = (ILubyte*)"8BPS";
	ILimage		*TempImage;
	ILpal		*TempPal;
	ILuint		c, i;
	ILubyte		*TempData;
	ILushort	*ShortPtr;
	ILenum		Format, Type;
	SIO *io;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;

	Format = image->Format;
	Type = image->Type;

	// All of these comprise the actual signature.
	SIOwrite(io, Signature, 1, 4);
	SaveBigShort(io,1);
	SaveBigInt(io,0);
	SaveBigShort(io,0);

	SaveBigShort(io,image->Bpp);
	SaveBigInt(io,image->Height);
	SaveBigInt(io,image->Width);
	if (image->Bpc > 2)
		Type = IL_UNSIGNED_SHORT;

	if (image->Format == IL_BGR)
		Format = IL_RGB;
	else if (image->Format == IL_BGRA)
		Format = IL_RGBA;

	if (Format != image->Format || Type != image->Type) {
		TempImage = iConvertImage(image, Format, Type);
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else {
		TempImage = image;
	}
	SaveBigShort(io,(ILushort)(TempImage->Bpc * 8));

	// @TODO:  Put the other formats here.
	switch (TempImage->Format)
	{
		case IL_COLOUR_INDEX:
			SaveBigShort(io,2);
			break;
		case IL_LUMINANCE:
			SaveBigShort(io,1);
			break;
		case IL_RGB:
		case IL_RGBA:
			SaveBigShort(io,3);
			break;
		default:
			iSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	if (TempImage->Format == IL_COLOUR_INDEX) {
		// @TODO: We're currently making a potentially fatal assumption that
		//	iConvertImage was not called if the format is IL_COLOUR_INDEX.
		TempPal = iConvertPal(&TempImage->Pal, IL_PAL_RGB24);
		if (TempPal == NULL)
			return IL_FALSE;
		SaveBigInt(io,768);

		// Have to save the palette in a planar format.
		for (c = 0; c < 3; c++) {
			for (i = c; i < TempPal->PalSize; i += 3) {
				SIOputc(io, TempPal->Palette[i]);
			}
		}

		ifree(TempPal->Palette);
	}
	else {
		SaveBigInt(io,0);  // No colour mode data.
	}

	SaveBigInt(io,0);  // No image resources.
	SaveBigInt(io,0);  // No layer information.
	SaveBigShort(io,0);  // Psd data, no compression.

	// @TODO:  Add RLE compression.

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			iCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	if (TempImage->Bpc == 1) {
		for (c = 0; c < TempImage->Bpp; c++) {
			for (i = c; i < TempImage->SizeOfPlane; i += TempImage->Bpp) {
				SIOputc(io, TempData[i]);
			}
		}
	}
	else {  // TempImage->Bpc == 2
		ShortPtr = (ILushort*)TempData;
		TempImage->SizeOfPlane /= 2;
		for (c = 0; c < TempImage->Bpp; c++) {
			for (i = c; i < TempImage->SizeOfPlane; i += TempImage->Bpp) {
				SaveBigUShort(io, ShortPtr[i]);
			}
		}
		TempImage->SizeOfPlane *= 2;
	}

	if (TempData != TempImage->Data)
		ifree(TempData);

	if (TempImage != image)
		iCloseImage(TempImage);


	return IL_TRUE;
}

ILconst_string iFormatExtsPSD[] = { 
  IL_TEXT("psd"), 
  IL_TEXT("pdd"), 
  NULL 
};

ILformat iFormatPSD = { 
  /* .Validate = */ iIsValidPsd, 
  /* .Load     = */ iLoadPsdInternal, 
  /* .Save     = */ iSavePsdInternal, 
  /* .Exts     = */ iFormatExtsPSD
};

#endif//IL_NO_PSD
