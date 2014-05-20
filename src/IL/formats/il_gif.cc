//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2008 by Denton Woods
// Copyright (C)      2014 by Björn Ganster
// Copyright (C)      2014 by Björn Paetzel
// Last modified: 2014 by Björn Paetzel
//
// Filename: src/IL/formats/il_gif.c
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//  The LZW decompression code is based on code released to the public domain
//    by Javier Arevalo and can be found at
//    http://www.programmersheaven.com/zone10/cat452 (* defunct)
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_GIF

#include "il_gif.h"
#include <stdio.h>

static ILboolean
GifCheckHeader(
	GifSignature *Sig
) {
	return memcmp(Sig->Magic, GIF_VERSION87A, 6) == 0
	    || memcmp(Sig->Magic, GIF_VERSION89A, 6) == 0;
}

static ILboolean
GifLoadColorTable(
	ILimage *	TargetImage,
	ILubyte   Size,
	ILpal *		Pal
) {
	if (Pal->Palette) {
		ifree(Pal->Palette);
	}

	Pal->PalSize = (1 << (Size+1)) * 4;
	Pal->PalType = IL_PAL_RGBA32;
	Pal->Palette = (ILubyte*)ialloc(Pal->PalSize);

	if (Pal->Palette == NULL) {
		Pal->PalSize = 0;
		return IL_FALSE;
	}

	ILubyte Temp[256*3];
	ILuint  PalSize = (1 << (Size + 1)) * 3;

	if (TargetImage->io.read(TargetImage->io.handle, Temp, 1, PalSize) != PalSize)
		return IL_FALSE;

	ILuint i;
	for (i=0; i<1<<(Size+1); i++) {
		Pal->Palette[i*4+0] = Temp[i*3+0];
		Pal->Palette[i*4+1] = Temp[i*3+1];
		Pal->Palette[i*4+2] = Temp[i*3+2];
		Pal->Palette[i*4+3] = 255;
	}

	return IL_TRUE;
}

static ILboolean
GifLoadSubBlock(
 	GifLoadingContext *					Ctx,
 	ILubyte *                   SubBlock,
 	ILubyte *                   Length
) {
	if (Ctx->Target->io.read(Ctx->Target->io.handle, Length, 1, 1) != 1) {
		return IL_FALSE;
	}

	if (Ctx->Target->io.read(Ctx->Target->io.handle, SubBlock, 1, *Length) != *Length) {
		return IL_FALSE;
	}

	return IL_TRUE;
}

static ILboolean
GifHandleGraphicControlExtension(
	GifLoadingContext *                 Ctx,
	const GifGraphicControlExtension *	Gce
) {
	ILushort Delay = Gce->Delay;
	UShort(Delay);

	Ctx->Delay 								= Delay * 10;
	Ctx->UseTransparentColor 	= Gce->Flags & GifFlag_GCE_TransparentColor;
	Ctx->TransparentColor    	= Gce->TransparentColor;
	Ctx->DisposalMethod      	= (Gce->Flags & GifFlag_GCE_DisposalMethodMask) >> GifFlag_GCE_DisposalMethodShift;

	// ILubyte Reserved = (Gce->Flags & GifFlag_GCE_ReservedMask) >> GifFlag_GCE_ReservedShift;

	return IL_TRUE;
}

static ILboolean
GifHandleExtensionSubBlock(
	GifLoadingContext *Ctx,
	ILubyte 				Type, 
	const ILubyte *	SubBlock, 
	ILubyte 				SubBlockLength,
	ILuint          SubBlockIndex
) {
	switch (Type) {
		case GifExt_PlainText:
		case GifExt_Comment: 
			char tmp[257];
			memcpy(tmp, SubBlock, SubBlockLength);
			tmp[SubBlockLength] = 0;
			return IL_TRUE;

		case GifExt_Application:
			return IL_TRUE;

		case GifExt_GraphicControl:
			if (SubBlockLength != sizeof(GifGraphicControlExtension)) {
				iTrace("**** GifGraphicControlExtension has wrong size %d expected %ld\n", SubBlockLength, sizeof(GifGraphicControlExtension));
				return IL_FALSE;
			}
			return GifHandleGraphicControlExtension(Ctx, (const GifGraphicControlExtension *)SubBlock);
	}
}

static ILboolean
GifLoadExtension(
 	GifLoadingContext *Ctx
) {
	ILubyte Type;
	if (Ctx->Target->io.read(Ctx->Target->io.handle, &Type, 1, 1) != 1) {
		return IL_FALSE;
	}

	ILubyte SubBlock[255];
	ILubyte SubBlockLength = 0;
	ILubyte SubBlockIndex = 0;
	do {
		if (!GifLoadSubBlock(Ctx, SubBlock, &SubBlockLength))
			return IL_FALSE;

		if (SubBlockLength > 0) {
			GifHandleExtensionSubBlock(Ctx, Type, SubBlock, SubBlockLength, SubBlockIndex);
			SubBlockIndex ++;
		}
	} while(SubBlockLength > 0);

	return IL_TRUE;
}

static ILboolean
GifHandleDisposal(
	GifLoadingContext *Ctx
) {
	Ctx->Image->Next = ilNewImage(Ctx->Target->Width, Ctx->Target->Height, 1, 1, 1);
	if (Ctx->Image->Next == NULL) {
		iTrace("**** failed to create new image for frame!\n");
		return IL_FALSE;
	}

	switch (Ctx->DisposalMethod) {
		case GifDisposal_DontCare:
		case GifDisposal_Overlay:
			memcpy(Ctx->Image->Next->Data, Ctx->Image->Data, Ctx->Image->SizeOfData);
			break;

		case GifDisposal_Clear:
			if (Ctx->UseTransparentColor) {
				memset(Ctx->Image->Next->Data, Ctx->Screen.Background, Ctx->Image->SizeOfData);
			} else {
				memset(Ctx->Image->Next->Data, Ctx->TransparentColor, Ctx->Image->SizeOfData);
			}
			break;

		case GifDisposal_Restore:
			if (Ctx->PrevImage){
				memcpy(Ctx->Image->Next->Data, Ctx->PrevImage, Ctx->Image->SizeOfData);
			} else if (Ctx->UseTransparentColor) {
				memset(Ctx->Image->Next->Data, Ctx->Screen.Background, Ctx->Image->SizeOfData);
			} else {
				memset(Ctx->Image->Next->Data, Ctx->TransparentColor, Ctx->Image->SizeOfData);
			}
			break;
	}

	Ctx->PrevImage      = Ctx->Image;
	Ctx->Image 					= Ctx->Image->Next;
	Ctx->Image->Format 	= IL_COLOUR_INDEX;
	Ctx->Image->Origin 	= IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}

static void 
LZWInputStreamClearPhrases(
	LZWInputStream *Stream
);

static void 
LZWInputStreamInit(
	LZWInputStream *Stream,
	ILubyte CodeSize
) {
	Stream->CodeSize 		= CodeSize+1;
	Stream->OriginalCodeSize = CodeSize+1;

	Stream->Data 				= (ILubyte*)ialloc(1);
	Stream->AllocCount 	= 1;
	Stream->BitCount 		= 0;

	Stream->ClearCode  	= 1 << CodeSize;
	Stream->EndCode  		= Stream->ClearCode + 1;

	Stream->PrevCode    = Stream->ClearCode;
	Stream->NextCode    = Stream->ClearCode + 2;

	memset(Stream->Phrases,	0, sizeof(Stream->Phrases));
	memset(Stream->Buffer, 	0, sizeof(Stream->Buffer));

	Stream->BufferLen   = 0;

	LZWInputStreamClearPhrases(Stream);
}

static void 
LZWInputStreamClearPhrases(
	LZWInputStream *Stream
) {
	ILuint i;

	Stream->CodeSize = Stream->OriginalCodeSize;

	for (i=0; i<4096; i++) {
		if (Stream->Phrases[i]) {
			if (i > Stream->EndCode) {
				ifree(Stream->Phrases[i]);
				Stream->Phrases[i] = NULL;
			}
		} else {
			if (i < Stream->ClearCode) {
				Stream->Phrases[i] = (ILuint*)ialloc(sizeof(ILuint)*2);
				Stream->Phrases[i][0] = 1;
				Stream->Phrases[i][1] = i;
			}
		}
	}

	Stream->PrevCode    = Stream->ClearCode;
	Stream->NextCode    = Stream->ClearCode + 2;
}

static void 
LZWInputStreamDeinit(
	LZWInputStream *Stream
) {
	if (Stream->Data) {
		ifree(Stream->Data);
	}

	ILuint i;
	for (i=0; i<4096; i++) {
		if (Stream->Phrases[i]) {
			ifree(Stream->Phrases[i]);
			Stream->Phrases[i] = NULL;
		}
	}

	memset(Stream, 0, sizeof(*Stream));
}

static ILboolean
LZWInputStreamAppend(
	LZWInputStream *Stream, 
	const ILubyte * Data, 
	ILuint          Length
) {
	ILuint newLength = Stream->BitCount / 8 + 1 + Length;
	if (newLength >= Stream->AllocCount) {
		ILubyte *newData = (ILubyte*)ialloc(newLength);
		if (!newData) {
			iTrace("**** Could not allocate %d bytes\n", newLength);
			return IL_FALSE;
		}

		memcpy(newData, Stream->Data, Stream->BitCount / 8 + 1);
		ifree(Stream->Data);
		Stream->Data = newData;
		Stream->AllocCount = newLength;
	}

	ILubyte Shift = Stream->BitCount % 8;

	if (!Shift) {
		memcpy(Stream->Data + (Stream->BitCount / 8), Data, Length);
		Stream->BitCount += 8 * Length;
		return IL_TRUE;
	}

	// clear last data byte, upper bits
	Stream->Data[Stream->BitCount/8    ] &= 0xFF >> (8-Shift);

	ILuint i;
	for (i = 0; i<Length; i++) {
		Stream->Data[Stream->BitCount/8    ] |= Data[i] << Shift;
		Stream->Data[Stream->BitCount/8 + 1]  = Data[i] >> (8-Shift);
		Stream->BitCount += 8;
	}

	return IL_TRUE;
}

static ILboolean
LZWInputStreamReadCode(
	LZWInputStream *Stream, 
	ILuint *        Code
) {
	ILubyte BitsLeftToRead 	= Stream->CodeSize;
	ILubyte Shift 					= 0;

	if (Stream->BitCount < BitsLeftToRead) {
		iTrace("**** Could not read %u bits, only got %u\n", BitsLeftToRead, Stream->BitCount);
		return IL_FALSE;
	}

	*Code = 0;

	while (BitsLeftToRead > 8) {
		*Code |= Stream->Data[0] << Shift;
		
		memmove(Stream->Data, Stream->Data + 1, Stream->BitCount / 8);

		Stream->BitCount -= 8;
		BitsLeftToRead -= 8;
		Shift += 8;
	}

	*Code |= (Stream->Data[0] & (0xFF >> (8-BitsLeftToRead)))<<Shift;

	// Shift remaining bits
	ILuint i;
	for (i = 1; i<Stream->BitCount/8; i++) {
		Stream->Data[i-1]  = Stream->Data[i-1]   >> BitsLeftToRead;
		Stream->Data[i-1] |= Stream->Data[i] << (8-BitsLeftToRead);
	}
	Stream->BitCount -= BitsLeftToRead;

	return IL_TRUE;
}

static ILboolean
LZWInputStreamBufferByte(
	LZWInputStream *Stream,
	ILubyte Byte 
) {
	if (Stream->BufferLen < sizeof(Stream->Buffer)) {
		Stream->Buffer[Stream->BufferLen++] = Byte;
		return IL_TRUE;
	}
	return IL_FALSE;
}

// output a code to the output buffer
static ILboolean
LZWInputStreamBufferCode(
	LZWInputStream *Stream,
	ILuint Code
) {
	if (Code < Stream->ClearCode) {
		// output directly
		return LZWInputStreamBufferByte(Stream, Code & 0xFF);
	}

	if (Code >= 4096) {
		// invalid code, gif only supports 12 bit code size
		return IL_FALSE;
	}

	const ILuint *Phrase = Stream->Phrases[Code];
	if (!Phrase) {
		// no phrase known for code
		return IL_FALSE;
	}

	// output phrase code by code
	ILuint i;
	ILuint PhraseLength = Phrase[0];
	for (i = 0; i<PhraseLength; i++) {
		if (!LZWInputStreamBufferCode(Stream, Phrase[i+1]))
			return IL_FALSE;
	}
	return IL_TRUE;
}

static ILboolean
LZWInputStreamGetFirstOfCode(
	LZWInputStream *Stream,
	ILuint Code,
	ILuint *First
) {
	if (Code >= 4096) {
		// invalid code, gif only supports 12 bit code size
		iTrace("***** %s: Code out of range: %04x\n", __PRETTY_FUNCTION__, Code);
		return IL_FALSE;
	}

	const ILuint *Phrase = Stream->Phrases[Code];
	if (!Phrase) {
		// no phrase known for code
		iTrace("***** %s: Phrase for code unknown: %04x\n", __PRETTY_FUNCTION__, Stream->PrevCode);
		return IL_FALSE;
	}

	*First = Phrase[1];
	return IL_TRUE;
}

static ILboolean
LZWInputStreamAppendCode(
	LZWInputStream *Stream,
	ILuint Code
) {
	if (Code >= 4096 || Stream->NextCode >= 4096) {
		// invalid code, gif only supports 12 bit code size
		iTrace("***** %s: Code out of range: %04x\n", __PRETTY_FUNCTION__, Code);
		return IL_FALSE;
	}

	if (Stream->Phrases[Stream->NextCode]) {
		// already in dictionary
		iTrace("***** %s: Next code already in dictionary: %04x\n", __PRETTY_FUNCTION__, Stream->NextCode);
		return IL_FALSE;
	}

	const ILuint *PrevPhrase = Stream->Phrases[Stream->PrevCode];
	if (!PrevPhrase) {
		// no previous phrase known
		iTrace("***** %s: Phrase for previous code unknown: %04x\n", __PRETTY_FUNCTION__, Stream->PrevCode);
		return IL_FALSE;
	}

	ILuint PrevLength = PrevPhrase[0];
	ILuint *Phrase = (ILuint *) ialloc(sizeof(ILuint) * ( 2 + PrevLength ));

	Phrase[0] = PrevLength + 1;

	ILuint i;
	for (i = 0; i<PrevLength; i++) {
		Phrase[i+1] = PrevPhrase[i+1];
	}
	Phrase[i+1] = Code;

	Stream->Phrases[Stream->NextCode++] = Phrase;

	return IL_TRUE;
}

static ILboolean
LZWInputStreamDecode(
	LZWInputStream *Stream,
	ILuint Code
) {
	if (Code >= 4096) {
		// invalid code, gif only supports 12 bit code size
		return IL_FALSE;
	}

  if (Stream->PrevCode == Stream->ClearCode) {
  	if (!LZWInputStreamBufferCode(Stream, Code)) return IL_FALSE;
  	Stream->PrevCode = Code;
  	return IL_TRUE;
  }


	// is CODE in the code table?
	const ILuint *Phrase = Stream->Phrases[Code];
	if (Phrase) {
		if (!LZWInputStreamBufferCode(Stream, Code)) return IL_FALSE;

		ILuint K;
		if (!LZWInputStreamGetFirstOfCode(Stream, Code, &K)) return IL_FALSE;
		if (!LZWInputStreamAppendCode(Stream, K)) return IL_FALSE;

		Stream->PrevCode = Code;
	} else {
		if (Stream->NextCode != Code) {
			iTrace("**** Code mismatch: %04x != %04x !!\n", Stream->NextCode, Code);
		}

		ILuint K;
		if (!LZWInputStreamGetFirstOfCode(Stream, Stream->PrevCode, &K)) return IL_FALSE;

		if (!LZWInputStreamBufferCode(Stream, Stream->PrevCode)) return IL_FALSE;
		if (!LZWInputStreamBufferCode(Stream, K)) return IL_FALSE;
		if (!LZWInputStreamAppendCode(Stream, K)) return IL_FALSE;

		Stream->PrevCode = Stream->NextCode-1;
	}

	if (Stream->NextCode == (1<<Stream->CodeSize) || Code == ((1<<Stream->CodeSize)-1)) {
		if (Stream->CodeSize < 12) {
			Stream->CodeSize++;
		} else {
			iTrace("**** Not ncreasing code size to %u\n", Stream->CodeSize + 1);
		}
	}

	return IL_TRUE;
}

static ILboolean
LZWInputStreamReadByte(
	LZWInputStream *Stream,
	ILubyte *Out
) {
	// decode code if necessary
	while (Stream->BufferLen == 0) {
		// get next code
		ILuint Code = 0;
		if (!LZWInputStreamReadCode(Stream, &Code)) {
			iTrace("**** Out of Codes\n");
			return IL_FALSE;
		}

		// reinitialize code table if requested
		while (Code == Stream->ClearCode) {
			LZWInputStreamClearPhrases(Stream);

			if (!LZWInputStreamReadCode(Stream, &Code)) {
				iTrace("**** Out of Codes after reset\n");
				return IL_FALSE;
			}
		}

		// handle end of image
		if (Code == Stream->EndCode) {
			iTrace("---- End of image\n");
			return IL_FALSE;
		}

		// handle code table entries
		if (!LZWInputStreamDecode(Stream, Code)) {
			iTrace("**** Could not decode\n");
			return IL_FALSE;
		}
	}

	// return decoded bytes 
	if (Stream->BufferLen > 0) {
		*Out = Stream->Buffer[0];
		memmove(Stream->Buffer, Stream->Buffer+1, Stream->BufferLen - 1);
		Stream->BufferLen -= 1;
		return IL_TRUE;
	}

	return IL_FALSE;
}

static ILboolean
GifLoadFrame(
 	GifLoadingContext *Ctx
) {
	GifImageDescriptor Img;
	if (Ctx->Target->io.read(Ctx->Target->io.handle, &Img, 1, sizeof(Img)) != sizeof(Img)) {
		iTrace("**** failed to read image descriptor\n");
		return IL_FALSE;
	}

	UShort(Img.Left);
	UShort(Img.Top);
	UShort(Img.Width);
	UShort(Img.Height);

	Ctx->UseLocalPal = Img.Flags & GifFlag_IMG_HasLocalColorTable;
	if (Ctx->UseLocalPal 
	&& !GifLoadColorTable(Ctx->Target, Img.Flags & GifFlag_IMG_LocalColorTableSizeMask, &Ctx->LocalPal)) {
		iTrace("**** Failed to load local palette\n");
		return IL_FALSE;
	}

	if (Ctx->UseLocalPal) {
		Ctx->Colors = 1<<(1+(Img.Flags & GifFlag_IMG_LocalColorTableSizeMask));
	} else {
		Ctx->Colors = 1<<(1+(Ctx->Screen.Flags & GifFlag_LSD_GlobalColorTableSizeMask));
	}

	Ctx->IsInterlaced = Img.Flags & GifFlag_IMG_Interlaced;

	if (Ctx->Frame) {
		if (!GifHandleDisposal(Ctx)) {
			iTrace("**** GifHandleDisposal failed!\n");
			return IL_FALSE;
		}
	}

	if (Ctx->UseLocalPal) {
		if (!iCopyPalette(&Ctx->Image->Pal, &Ctx->LocalPal))
			return IL_FALSE;
	} else {
		if (!iCopyPalette(&Ctx->Image->Pal, &Ctx->GlobalPal))
			return IL_FALSE;
	}
	
	if (Ctx->UseTransparentColor && Ctx->TransparentColor < Ctx->Image->Pal.PalSize/4) {
		Ctx->Image->Pal.Palette[Ctx->TransparentColor * 4 + 3] = 0;
	}

	if (Ctx->Target->io.read(Ctx->Target->io.handle, &Ctx->LZWCodeSize, 1, 1) != 1) {
		return IL_FALSE;
	}

	if (Ctx->LZWCodeSize < 2 || Ctx->LZWCodeSize > 12) {
		iTrace("**** Lzw Code size out of range!\n");
		return IL_FALSE;
	}

	ILubyte SubBlock[255];
	ILubyte SubBlockLength = 0;

	LZWInputStream Stream;
	LZWInputStreamInit(&Stream, Ctx->LZWCodeSize);

	do {
		if (!GifLoadSubBlock(Ctx, SubBlock, &SubBlockLength)) {
			ifree(Stream.Data);
			return IL_FALSE;
		}

		if (SubBlockLength) {
			if (!LZWInputStreamAppend(&Stream, SubBlock, SubBlockLength)) {
				break;
			}
		}
	} while(SubBlockLength > 0);

	ILubyte B;
	ILuint y = Img.Top;
	ILuint x = Img.Left;
	ILubyte interlaceGroup = 1;
	while(LZWInputStreamReadByte(&Stream, &B) && interlaceGroup < 5) {
		B %= Ctx->Colors;

		if ( x < Ctx->Screen.Width 
			&& y < Ctx->Screen.Height 
			&& ( B != Ctx->TransparentColor 
				|| !Ctx->UseTransparentColor 
				|| Ctx->DisposalMethod != GifDisposal_Overlay 
				)
		) {
			Ctx->Image->Data[x + y * Ctx->Screen.Width] = B;
		}

		x++;
		if (x == Img.Width + Img.Left) {
			x = Img.Left;

			if (Ctx->IsInterlaced) {
				if (interlaceGroup == 1 || interlaceGroup == 2) {
					y += 8;
				} else if (interlaceGroup == 3) {
					y += 4;
				} else if (interlaceGroup == 4) {
					y += 2;
				}

				if (y >= Img.Height) {
					interlaceGroup++;
					if (interlaceGroup == 2) {
						y = Img.Top + 4;
					} else if (interlaceGroup == 3) {
						y = Img.Top + 2;
					} else if (interlaceGroup == 4) {
						y = Img.Top + 1;
					}
				}

			} else {
				y++;
			}

			if (y == Ctx->Screen.Height)
				break;
		}
	}

	LZWInputStreamDeinit(&Stream);

	Ctx->Frame ++;
	return IL_TRUE;
}

static ILboolean
GifLoad(
	GifLoadingContext *Ctx
) {
	ILubyte ID;
	if (Ctx->Target->io.read(Ctx->Target->io.handle, &ID, 1, 1) != 1) {
		return IL_FALSE;
	}

	while (ID != GifID_End) {
		switch (ID) {
			case GifID_Extension:
				if (!GifLoadExtension(Ctx))
					return Ctx->Frame > 0;
				break;

			case GifID_Image:
				if (!GifLoadFrame(Ctx))
					return Ctx->Frame > 0;
				break;

			default:
				return Ctx->Frame > 0;
		}

		// try to read next block id
		if (Ctx->Target->io.read(Ctx->Target->io.handle, &ID, 1, 1) != 1) {
			break;
		}
	}

	/*
	if (Ctx->IsInterlaced) {
		Ctx->Image = Ctx->Target;
		while(Ctx->Image) {
			GifDeinterlace(Ctx->Image);
			Ctx->Image = Ctx->Image->Next;
		}
	}
	*/
	return IL_TRUE;
}

// Internal function used to load the Gif.
ILboolean iLoadGifInternal(ILimage* TargetImage)
{
	if (TargetImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	GifLoadingContext Ctx;
	memset(&Ctx, 0, sizeof(Ctx));
	Ctx.Target = TargetImage;
	Ctx.Image  = TargetImage;

	// check for valid signature
	GifSignature Sig;

	if (Ctx.Target->io.read(Ctx.Target->io.handle, &Sig, 1, sizeof(Sig)) != sizeof(Sig)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!GifCheckHeader(&Sig)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// get screen descriptor
	if (Ctx.Target->io.read(Ctx.Target->io.handle, &Ctx.Screen, 1, sizeof(Ctx.Screen)) != sizeof(Ctx.Screen)) {
		ilSetError(IL_FILE_READ_ERROR);
		return IL_FALSE;
	}

	// swap bytes if necessary
	UShort(Ctx.Screen.Width);
	UShort(Ctx.Screen.Height);

	// create texture
	if (!ilTexImage(Ctx.Screen.Width, Ctx.Screen.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	// check for a global color table
	if (Ctx.Screen.Flags & GifFlag_LSD_HasGlobalColorTable 
	&& !GifLoadColorTable(Ctx.Target, Ctx.Screen.Flags & GifFlag_LSD_GlobalColorTableSizeMask, &Ctx.GlobalPal))
		return IL_FALSE;

	// try loading image(s)
	ILboolean Loaded = GifLoad(&Ctx);

	// always clean up palette afterwards
	if (Ctx.GlobalPal.Palette && Ctx.GlobalPal.PalSize)
		ifree(Ctx.GlobalPal.Palette);
	Ctx.GlobalPal.Palette = NULL;
	Ctx.GlobalPal.PalSize = 0;

	if (Ctx.LocalPal.Palette && Ctx.LocalPal.PalSize)
		ifree(Ctx.LocalPal.Palette);
	Ctx.LocalPal.Palette = NULL;
	Ctx.LocalPal.PalSize = 0;

	return Loaded && ilFixImage();
}

// Internal function to get the header and check it.
ILboolean 
iIsValidGif(SIO* io) {
	GifSignature Sig;
	ILint64 Read = io->read(io->handle, &Sig, 1, 6);
	io->seek(io->handle, -Read, IL_SEEK_CUR);
	return Read == 6 && GifCheckHeader(&Sig);
}

#endif //IL_NO_GIF
