//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pic.c
//
// Description: Softimage Pic (.pic) functions
//	Lots of this code is taken from Paul Bourke's Softimage Pic code at
//	http://local.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_PIC
#include "il_pic.h"
#include "il_manip.h"
#include <string.h>


// Internal function used to get the .pic header from the current file.
static ILuint iGetPicHead(SIO* io, PIC_HEAD *Header)
{
	ILuint read = io->read(io->handle, Header, 1, sizeof(*Header));

	Int  (&Header->Magic);
	Float(&Header->Version);
	Short(&Header->Width);
	Short(&Header->Height);
	Float(&Header->Ratio);
	Short(&Header->Fields);
	Short(&Header->Padding);

	return read;
}


// Internal function used to check if the header is a valid .pic header.
static ILboolean iCheckPic(PIC_HEAD *Header)
{
	if (Header->Magic != 0x5380F634)
		return IL_FALSE;
	if (strncmp((const char*)Header->Id, "PICT", 4))
		return IL_FALSE;
	if (Header->Width == 0)
		return IL_FALSE;
	if (Header->Height == 0)
		return IL_FALSE;

	return IL_TRUE;
}


// Internal function to get the header and check it.
static ILboolean iIsValidPic(SIO* io)
{
	PIC_HEAD	Head;
	ILuint Start = SIOtell(io);
	ILuint read = iGetPicHead(io, &Head);
	SIOseek(io, Start, IL_SEEK_SET); // Go ahead and restore to previous state
	return read == sizeof(Head) && iCheckPic(&Head);
}


static ILboolean channelReadMixed(SIO* io, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILint	count;
	int		i, j, k;
	ILubyte	col[4];

	for(i = 0; i < width; i += count) {
		if (io->eof(io->handle))
			return IL_FALSE;

		count = io->getchar(io->handle);
		if (count == IL_EOF)
			return IL_FALSE;

		if (count >= 128) {  // Repeated sequence
			if (count == 128) {  // Long run
				count = GetBigUShort(io);
				if (io->eof(io->handle)) {
					iSetError(IL_FILE_READ_ERROR);
					return IL_FALSE;
				}
			}
			else
				count -= 127;
			
			// We've run past...
			if ((i + count) > width) {
				//fprintf(stderr, "ERROR: FF_PIC_load(): Overrun scanline (Repeat) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				iSetError(IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}

			for (j = 0; j < noCol; j++)
				if (io->read(io->handle, &col[j], 1, 1) != 1) {
					iSetError(IL_FILE_READ_ERROR);
					return IL_FALSE;
				}

			for (k = 0; k < count; k++, scan += bytes) {
				for (j = 0; j < noCol; j++)
					scan[off[j]] = col[j];
			}
		} else {				// Raw sequence
			count++;
			if ((i + count) > width) {
				//fprintf(stderr, "ERROR: FF_PIC_load(): Overrun scanline (Raw) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				iSetError(IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}
			
			for (k = count; k > 0; k--, scan += bytes) {
				for (j = 0; j < noCol; j++)
					if (io->read(io->handle, &scan[off[j]], 1, 1) != 1) {
						iSetError(IL_FILE_READ_ERROR);
						return IL_FALSE;
					}
			}
		}
	}

	return IL_TRUE;
}


static ILboolean channelReadRaw(SIO* io, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILint i, j;

	for (i = 0; i < width; i++) {
		if (io->eof(io->handle))
			return IL_FALSE;
		for (j = 0; j < noCol; j++)
			if (io->read(io->handle, &scan[off[j]], 1, 1) != 1)
				return IL_FALSE;
		scan += bytes;
	}
	return IL_TRUE;
}


static ILboolean channelReadPure(SIO* io, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILubyte		col[4];
	ILint		count;
	int			i, j, k;

	for (i = width; i > 0; ) {
		count = io->getchar(io->handle);
		if (count == IL_EOF)
			return IL_FALSE;
		if (count > width)
			count = width;
		i -= count;
		
		if (io->eof(io->handle))
			return IL_FALSE;
		
		for (j = 0; j < noCol; j++)
			if (io->read(io->handle, &col[j], 1, 1) != 1)
				return IL_FALSE;
		
		for (k = 0; k < count; k++, scan += bytes) {
			for(j = 0; j < noCol; j++)
				scan[off[j] + k] = col[j];
		}
	}
	return IL_TRUE;
}


static ILuint readScanline(SIO* io, ILubyte *scan, ILint width, CHANNEL *channel, ILint bytes)
{
	ILint		noCol;
	ILint		off[4] = {0,0,0,0};
	ILuint		status=0;

	while (channel) {
		noCol = 0;
		if(channel->Chan & PIC_RED_CHANNEL) {
			off[noCol] = 0;
			noCol++;
		}
		if(channel->Chan & PIC_GREEN_CHANNEL) {
			off[noCol] = 1;
			noCol++;
		}
		if(channel->Chan & PIC_BLUE_CHANNEL) {
			off[noCol] = 2;
			noCol++;
		}
		if(channel->Chan & PIC_ALPHA_CHANNEL) {
			off[noCol] = 3;
			noCol++;
			//@TODO: Find out if this is possible.
			if (bytes == 3)  // Alpha channel in a 24-bit image.  Do not know what to do with this.
				return 0;
		}

		if (!noCol)
			return 0;

		switch(channel->Type & 0x0F)
		{
			case PIC_UNCOMPRESSED:
				status = channelReadRaw(io, scan, width, noCol, off, bytes);
				break;
			case PIC_PURE_RUN_LENGTH:
				status = channelReadPure(io, scan, width, noCol, off, bytes);
				break;
			case PIC_MIXED_RUN_LENGTH:
				status = channelReadMixed(io, scan, width, noCol, off, bytes);
				break;
		}
		if (!status)
			break;

		channel = (CHANNEL*)channel->Next;
	}
	return status;
}

static ILboolean readScanlines(SIO* io, ILuint *image, ILint width, ILint height, CHANNEL *channel, ILuint alpha)
{
	ILint	i;

	for (i = height - 1; i >= 0; i--) {
		ILuint	*scan = image + i * width;

		if (!readScanline(io, (ILubyte *)scan, width, channel, alpha ? 4 : 3)) {
			iSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}


// Internal function used to load the .pic
static ILboolean iLoadPicInternal(ILimage* image)
{
	ILuint		Alpha = IL_FALSE;
	ILubyte		Chained;
	CHANNEL		*Channel = NULL, *Channels = NULL, *Prev;
	PIC_HEAD	Header;
	ILboolean	Read;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPicHead(&image->io, &Header))
		return IL_FALSE;
	if (!iCheckPic(&Header)) {
		iSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Read channels
	do {
		if (Channel == NULL) {
			Channel = Channels = (CHANNEL*)ialloc(sizeof(CHANNEL));
			if (Channels == NULL)
				return IL_FALSE;
		}
		else {
			Channels->Next = (CHANNEL*)ialloc(sizeof(CHANNEL));
			if (Channels->Next == NULL) {
				// Clean up the list before erroring out.
				while (Channel) {
					Prev = Channel;
					Channel = (CHANNEL*)Channel->Next;
					ifree(Prev);
				}
				return IL_FALSE;
			}
			Channels = (CHANNEL*)Channels->Next;
		}
		Channels->Next = NULL;

		Chained = (ILubyte)SIOgetc(&image->io);
		Channels->Size = (ILubyte)SIOgetc(&image->io);
		Channels->Type = (ILubyte)SIOgetc(&image->io);
		Channels->Chan = (ILubyte)SIOgetc(&image->io);
		if (SIOeof(&image->io)) {
			Read = IL_FALSE;
			goto finish;
		}
		
		// See if we have an alpha channel in there
		if (Channels->Chan & PIC_ALPHA_CHANNEL)
			Alpha = IL_TRUE;
		
	} while (Chained);

	if (Alpha) {  // Has an alpha channel
		if (!iTexImage(image, Header.Width, Header.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
			Read = IL_FALSE;
			goto finish;  // Have to destroy Channels first.
		}
	}
	else {  // No alpha channel
		if (!iTexImage(image, Header.Width, Header.Height, 1, 3, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
			Read = IL_FALSE;
			goto finish;  // Have to destroy Channels first.
		}
	}
	image->Origin = IL_ORIGIN_LOWER_LEFT;

	Read = readScanlines(&image->io, (ILuint*)(void*)image->Data, Header.Width, Header.Height, Channel, Alpha);

finish:
	// Destroy channels
	while (Channel) {
		Prev = Channel;
		Channel = (CHANNEL*)Channel->Next;
		ifree(Prev);
	}

	if (Read == IL_FALSE)
		return IL_FALSE;

	return IL_TRUE;
}

static ILconst_string iFormatExtsPIC[] = { 
  IL_TEXT("pic"), 
  NULL 
};

ILformat iFormatPIC = { 
  /* .Validate = */ iIsValidPic, 
  /* .Load     = */ iLoadPicInternal, 
  /* .Save     = */ NULL, 
  /* .Exts     = */ iFormatExtsPIC
};

#endif//IL_NO_PIC

