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
	ILubyte Temp[256*3];
	ILuint  PalSize = (1 << (Size + 1)) * 3;
    ILint i;

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

	if (TargetImage->io.read(TargetImage->io.handle, Temp, 1, PalSize) != PalSize)
		return IL_FALSE;

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
	UShort(&Delay);

	Ctx->Delay 								= Delay * 10;
	Ctx->UseTransparentColor 	= Gce->Flags & GifFlag_GCE_TransparentColor;
	Ctx->TransparentColor    	= Gce->TransparentColor;
	Ctx->NextDisposalMethod   = (Gce->Flags & GifFlag_GCE_DisposalMethodMask) >> GifFlag_GCE_DisposalMethodShift;

	// ILubyte Reserved = (Gce->Flags & GifFlag_GCE_ReservedMask) >> GifFlag_GCE_ReservedShift;

	return IL_TRUE;
}

static ILboolean
GifHandleExtensionSubBlock(
	GifLoadingContext *Ctx,
	ILubyte 				Type, 
	const ILubyte *	SubBlock, 
	ILubyte 				SubBlockLength
) {
	switch (Type) {
		case GifExt_PlainText:
		case GifExt_Comment: {
			char tmp[257];
			memcpy(tmp, SubBlock, SubBlockLength);
			tmp[SubBlockLength] = 0;
			return IL_TRUE;
		}

		case GifExt_Application:
			return IL_TRUE;

		case GifExt_GraphicControl:
			if (SubBlockLength != sizeof(GifGraphicControlExtension)) {
				iTrace("**** GifGraphicControlExtension has wrong size %u expected %u\n", SubBlockLength, (ILuint)sizeof(GifGraphicControlExtension));
				return IL_FALSE;
			}
			return GifHandleGraphicControlExtension(Ctx, (const GifGraphicControlExtension *)SubBlock);
	}

	return IL_TRUE;
}

static ILboolean
GifLoadExtension(
 	GifLoadingContext *Ctx
) {
	ILubyte Type;
	ILubyte SubBlock[255];
	ILubyte SubBlockLength = 0;

	if (Ctx->Target->io.read(Ctx->Target->io.handle, &Type, 1, 1) != 1) {
		return IL_FALSE;
	}

	do {
		if (!GifLoadSubBlock(Ctx, SubBlock, &SubBlockLength))
			return IL_FALSE;

		if (SubBlockLength > 0) {
			GifHandleExtensionSubBlock(Ctx, Type, SubBlock, SubBlockLength);
		}
	} while(SubBlockLength > 0);

	return IL_TRUE;
}

static ILboolean
GifHandleDisposal(
	GifLoadingContext *Ctx
) {
	Ctx->Image->Next = iNewImageFull(Ctx->Target->Width, Ctx->Target->Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
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
			memset(Ctx->Image->Next->Data, 0, Ctx->Image->SizeOfData);
			break;

		case GifDisposal_Restore:
			if (Ctx->PrevImage){
				memcpy(Ctx->Image->Next->Data, Ctx->PrevImage->Data, Ctx->Image->SizeOfData);
			} else  {
				memset(Ctx->Image->Next->Data, 0, Ctx->Image->SizeOfData);
			}
			break;
	}

	Ctx->DisposalMethod = Ctx->NextDisposalMethod;

	Ctx->PrevImage      = Ctx->Image;
	Ctx->Image 					= Ctx->Image->Next;
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
	ILubyte CodeSize,
	GifLoadingContext *Ctx
) {
	Stream->Ctx         			= Ctx;

	Stream->CodeSize 					= CodeSize+1;
	Stream->OriginalCodeSize 	= CodeSize+1;

	Stream->InputAvail 				= 0;
	Stream->InputPos          = 0;
	Stream->InputBits         = 0;
	Stream->InputBitCount 		= 0;

	Stream->ClearCode  				= 1 << CodeSize;
	Stream->EndCode  					= Stream->ClearCode + 1;

	Stream->PrevCode    			= Stream->ClearCode;
	Stream->NextCode    			= Stream->ClearCode + 2;

	memset(Stream->Phrases,				0, sizeof(Stream->Phrases));
	memset(Stream->OutputBuffer, 	0, sizeof(Stream->OutputBuffer));

	Stream->OutputBufferLen   = 0;

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
LZWInputStreamReadCode(
	LZWInputStream *Stream, 
	ILuint *        Code
) {
	ILubyte BitsLeftToRead 	= Stream->CodeSize;

	while (Stream->InputBitCount < BitsLeftToRead) {
		// make sure there is something in the input buffer
		if (Stream->InputAvail == 0) {
			Stream->InputPos = 0;
			if ( !GifLoadSubBlock(Stream->Ctx, Stream->InputBuffer, &Stream->InputAvail) 
				|| Stream->InputAvail == 0) {
				iTrace("**** Could not read %u bits, only got %u and could not load more", BitsLeftToRead, Stream->InputBitCount);
				return IL_FALSE;
			}
		}

		// get bits from input buffer, one byte at a time
		while (Stream->InputAvail > 0 && Stream->InputBitCount <= 24) {
			Stream->InputBits |= Stream->InputBuffer[Stream->InputPos] << Stream->InputBitCount;
			Stream->InputPos ++;
			Stream->InputAvail --;
			Stream->InputBitCount += 8;
		}
	}

	*Code = Stream->InputBits & ((1 << BitsLeftToRead) - 1);
	Stream->InputBitCount -= BitsLeftToRead;
	Stream->InputBits = (Stream->InputBits >> BitsLeftToRead);// & ((1<<Stream->InputBitCount)-1);

	return IL_TRUE;
}

static ILboolean
LZWInputStreamBufferByte(
	LZWInputStream *Stream,
	ILubyte Byte 
) {
	if (Stream->OutputBufferLen >= sizeof(Stream->OutputBuffer)) {
		iTrace("**** Output buffer overflow");
		return IL_FALSE;
	}
	Stream->OutputBuffer[Stream->OutputBufferLen++] = Byte;
	return IL_TRUE;
}

// output a code to the output buffer
static ILboolean
LZWInputStreamBufferCode(
	LZWInputStream *Stream,
	ILuint Code
) {
	const ILuint *Phrase;
	ILuint i;
	ILuint PhraseLength;

	if (Code < Stream->ClearCode) {
		// output directly
		return LZWInputStreamBufferByte(Stream, Code & 0xFF);
	}

	if (Code >= 4096) {
		// invalid code, gif only supports 12 bit code size
		return IL_FALSE;
	}

	Phrase = Stream->Phrases[Code];
	if (!Phrase) {
		// no phrase known for code
		return IL_FALSE;
	}

	// output phrase code by code
	PhraseLength = Phrase[0];
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
	const ILuint *Phrase;

	if (Code >= 4096) {
		// invalid code, gif only supports 12 bit code size
		iTrace("***** %s: Code out of range: %04x\n", __PRETTY_FUNCTION__, Code);
		return IL_FALSE;
	}

	Phrase = Stream->Phrases[Code];

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

	ILuint PrevLength;
	ILuint *Phrase;
    const ILuint *PrevPhrase;
	ILuint i;

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

	PrevPhrase = Stream->Phrases[Stream->PrevCode];
	if (!PrevPhrase) {
		// no previous phrase known
		iTrace("***** %s: Phrase for previous code unknown: %04x\n", __PRETTY_FUNCTION__, Stream->PrevCode);
		return IL_FALSE;
	}

  	PrevLength = PrevPhrase[0];
	Phrase = (ILuint *) ialloc(sizeof(ILuint) * ( 2 + PrevLength ));

	Phrase[0] = PrevLength + 1;

	for (i = 0; i<PrevLength; i++) {
		Phrase[i+1] = PrevPhrase[i+1];
	}
	Phrase[i+1] = Code;

	Stream->Phrases[Stream->NextCode++] = Phrase;

	if ( Stream->NextCode == (ILuint)(1<<Stream->CodeSize) ) {
		if (Stream->CodeSize < 12) {
			Stream->CodeSize++;
		} else {
			iTrace("**** Not increasing code size to %u @ %08x", Stream->CodeSize + 1, SIOtell(&Stream->Ctx->Target->io)-1-Stream->InputAvail);
		}
	}

	return IL_TRUE;
}

static ILboolean
LZWInputStreamDecode(
	LZWInputStream *Stream,
	ILuint Code
) {
    const ILuint *Phrase;

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
	Phrase = Stream->Phrases[Code];
	if (Phrase) {
		ILuint K;
		if (!LZWInputStreamBufferCode(Stream, Code)) return IL_FALSE;

		if (!LZWInputStreamGetFirstOfCode(Stream, Code, &K)) return IL_FALSE;

		// if dictionary is not full, create new entry
		if (Stream->NextCode < 4096 && !LZWInputStreamAppendCode(Stream, K)) return IL_FALSE;

		Stream->PrevCode = Code;
	} else {
		ILuint K;
		if (Stream->NextCode != Code) {
			iTrace("**** Code mismatch: %04x != %04x !!\n", Stream->NextCode, Code);
		}

		if (!LZWInputStreamGetFirstOfCode(Stream, Stream->PrevCode, &K)) return IL_FALSE;

		if (!LZWInputStreamBufferCode(Stream, Stream->PrevCode)) return IL_FALSE;
		if (!LZWInputStreamBufferCode(Stream, K)) return IL_FALSE;

		// if dictionary is not full, create new entry
		if (Stream->NextCode < 4096 && !LZWInputStreamAppendCode(Stream, K)) return IL_FALSE;

		Stream->PrevCode = Stream->NextCode-1;
	}
	return IL_TRUE;
}

static ILboolean
LZWInputStreamReadByte(
	LZWInputStream *Stream,
	ILubyte *Out
) {
	// decode code if necessary
	while (Stream->OutputBufferLen == 0) {
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
			return IL_FALSE;
		}

		// handle code table entries
		if (!LZWInputStreamDecode(Stream, Code)) {
			iTrace("**** Could not decode\n");
			Stream->InputAvail = 0;
			//return IL_FALSE;
		}
	}

	// return decoded bytes TODO: optimize out memmove
	if (Stream->OutputBufferLen > 0) {
		*Out = Stream->OutputBuffer[0];
		memmove(Stream->OutputBuffer, Stream->OutputBuffer+1, Stream->OutputBufferLen - 1);
		Stream->OutputBufferLen -= 1;
		return IL_TRUE;
	}

	return IL_FALSE;
}

static ILboolean
GifLoadFrame(
 	GifLoadingContext *Ctx
) {
	GifImageDescriptor Img;
	LZWInputStream Stream;
	ILubyte B;
	ILuint y;
	ILuint x;
	ILubyte interlaceGroup = 1;
	ILpal *SourcePal;

	if (Ctx->Target->io.read(Ctx->Target->io.handle, &Img, 1, sizeof(Img)) != sizeof(Img)) {
		iTrace("**** failed to read image descriptor\n");
		return IL_FALSE;
	}

	UShort(&Img.Left);
	UShort(&Img.Top);
	UShort(&Img.Width);
	UShort(&Img.Height);

	Ctx->UseLocalPal = Img.Flags & GifFlag_IMG_HasLocalColorTable;
	if (Ctx->UseLocalPal 
	&& !GifLoadColorTable(Ctx->Target, Img.Flags & GifFlag_IMG_LocalColorTableSizeMask, &Ctx->LocalPal)) {
		iTrace("**** Failed to load local palette\n");
		return IL_FALSE;
	}
	iTrace("---- UseLocalPal: %d", Ctx->UseLocalPal);
	iTrace("---- DisposalMethod: %d %d", Ctx->DisposalMethod, Ctx->Frame);
	iTrace("---- Delay: %d", Ctx->Delay);

	Ctx->IsInterlaced = Img.Flags & GifFlag_IMG_Interlaced;

	if (Ctx->Frame) {
		if (!GifHandleDisposal(Ctx)) {
			iTrace("**** GifHandleDisposal failed!\n");
			return IL_FALSE;
		}
	} else {
		Ctx->Image->Origin 	= IL_ORIGIN_UPPER_LEFT;
	}

	Ctx->Image->Duration = Ctx->Delay;

    SourcePal = Ctx->UseLocalPal ? &Ctx->LocalPal : &Ctx->GlobalPal;
	/*if (!iCopyPalette(&Ctx->Image->Pal, SourcePal)) {
		iTrace("**** Could not copy palette for frame!");
		return IL_FALSE;
	}*/

	if (Ctx->UseLocalPal) {
		Ctx->Colors = 1<<(1+(Img.Flags & GifFlag_IMG_LocalColorTableSizeMask));
	} else {
		Ctx->Colors = 1<<(1+(Ctx->Screen.Flags & GifFlag_LSD_GlobalColorTableSizeMask));
	}
	
	/*if (Ctx->UseTransparentColor && Ctx->TransparentColor < Ctx->Image->Pal.PalSize/4) {
		Ctx->Image->Pal.Palette[Ctx->TransparentColor * 4 + 3] = 0;
	}*/

	if (Ctx->Target->io.read(Ctx->Target->io.handle, &Ctx->LZWCodeSize, 1, 1) != 1) {
		iTrace("**** Could not read LZW code size!");
		return IL_FALSE;
	}

	if (Ctx->LZWCodeSize < 2 || Ctx->LZWCodeSize > 12) {
		iTrace("**** LZW code size out of range: %u", Ctx->LZWCodeSize);
		return IL_FALSE;
	}

	LZWInputStreamInit(&Stream, Ctx->LZWCodeSize, Ctx);

	y = Img.Top;
    x = Img.Left;

	while(LZWInputStreamReadByte(&Stream, &B) && interlaceGroup < 5) {
		B %= Ctx->Colors;

		if ( x < Ctx->Screen.Width 
			&& y < Ctx->Screen.Height 
			&& ( B != Ctx->TransparentColor 
				|| !Ctx->UseTransparentColor 
				|| Ctx->Frame == 0
				)
		) {
			((ILuint*)Ctx->Image->Data)[x + y * Ctx->Screen.Width] = ((ILuint*)SourcePal->Palette)[B];
			if (!Ctx->Frame && B == Ctx->TransparentColor && Ctx->UseTransparentColor) {
				Ctx->Image->Data[(x + y * Ctx->Screen.Width)*4+3] = 0;
			}
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

static void 
GifFixLastDisposal(
	GifLoadingContext *Ctx
) {
	if (Ctx->DisposalMethod == GifDisposal_Overlay) {
		// TODO: set all transparent pixels of first frame to values of pixels in last frame
	}
}

static ILboolean
GifLoad(
	GifLoadingContext *Ctx
) {
	ILubyte ID;
	if (SIOread(&Ctx->Target->io, &ID, 1, 1) != 1) {
		return IL_FALSE;
	}

	iTrace("---- Block ID %02x @ %08x", ID, SIOtell(&Ctx->Target->io)-1);
	while (ID != GifID_End) {
		switch (ID) {
			case GifID_Extension:
				if (!GifLoadExtension(Ctx))
					return Ctx->Frame > 0;
				break;

			case GifID_Image:
				if (!GifLoadFrame(Ctx)) {
					if (Ctx->Frame > 1)
						GifFixLastDisposal(Ctx);
					return Ctx->Frame > 0;
				}
				break;

			case GifID_Terminator:
				break;

			default:
				iTrace("---- Unknown block @ %08x", SIOtell(&Ctx->Target->io)-1);

				// just read next byte and hope for the best
				if (SIOread(&Ctx->Target->io, &ID, 1, 1) != 1) {
					return Ctx->Frame > 0;
				}
		}

		// try to read next block id
		if (SIOread(&Ctx->Target->io, &ID, 1, 1) != 1) {
			break;
		}
		iTrace("---- Block ID %02x @ %08x", ID, SIOtell(&Ctx->Target->io)-1);
	}
	return IL_TRUE;
}

// Internal function used to load the Gif.
ILboolean iLoadGifInternal(ILimage* TargetImage)
{
	GifLoadingContext Ctx;
	GifSignature Sig;
    ILboolean Loaded;

	if (TargetImage == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	memset(&Ctx, 0, sizeof(Ctx));
	Ctx.Target = TargetImage;
	Ctx.Image  = TargetImage;

	// check for valid signature

	if (Ctx.Target->io.read(Ctx.Target->io.handle, &Sig, 1, sizeof(Sig)) != sizeof(Sig)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!GifCheckHeader(&Sig)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// get screen descriptor
	if (Ctx.Target->io.read(Ctx.Target->io.handle, &Ctx.Screen, 1, sizeof(Ctx.Screen)) != sizeof(Ctx.Screen)) {
		iSetError(IL_FILE_READ_ERROR);
		return IL_FALSE;
	}

	// swap bytes if necessary
	UShort(&Ctx.Screen.Width);
	UShort(&Ctx.Screen.Height);

	// create texture
	if (!iTexImage(TargetImage, Ctx.Screen.Width, Ctx.Screen.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	// check for a global color table
	if (Ctx.Screen.Flags & GifFlag_LSD_HasGlobalColorTable 
	&& !GifLoadColorTable(Ctx.Target, Ctx.Screen.Flags & GifFlag_LSD_GlobalColorTableSizeMask, &Ctx.GlobalPal))
		return IL_FALSE;

	// try loading image(s)
	Loaded = GifLoad(&Ctx);

	// always clean up palette afterwards
	if (Ctx.GlobalPal.Palette && Ctx.GlobalPal.PalSize)
		ifree(Ctx.GlobalPal.Palette);
	Ctx.GlobalPal.Palette = NULL;
	Ctx.GlobalPal.PalSize = 0;

	if (Ctx.LocalPal.Palette && Ctx.LocalPal.PalSize)
		ifree(Ctx.LocalPal.Palette);
	Ctx.LocalPal.Palette = NULL;
	Ctx.LocalPal.PalSize = 0;

	return Loaded && IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean 
iIsValidGif(SIO* io) {
	GifSignature Sig;
	ILint Read = io->read(io->handle, &Sig, 1, 6);
	io->seek(io->handle, -Read, IL_SEEK_CUR);
	return Read == 6 && GifCheckHeader(&Sig);
}

ILconst_string iFormatExtsGIF[] = { 
	IL_TEXT("gif"), 
	NULL 
};

ILformat iFormatGIF = { 
	/* .Validate = */ iIsValidGif, 
	/* .Load     = */ iLoadGifInternal, 
	/* .Save     = */ NULL, 
	/* .Exts     = */ iFormatExtsGIF
};

#endif //IL_NO_GIF
