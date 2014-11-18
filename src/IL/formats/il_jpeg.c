//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_jpeg.c
//
// Description: Jpeg (.jpg) functions
//
//-----------------------------------------------------------------------------
//
// Most of the comments here are sufficient, as we're just using libjpeg.
// I have left most of the libjpeg example's comments intact, though.
//

// 2014-07-08: ijl based implementation removed as it is no longer supported
//             by intel

#include "il_internal.h"

#ifndef IL_NO_JPG

ILboolean iExifLoad(ILimage *Image);
ILboolean iExifSave(ILimage *Image);

#include "il_jpeg.h"
#include "il_manip.h"
#include <setjmp.h>

#include "pack_push.h"
typedef struct {
	ILboolean 	jpgErrorOccured; // = IL_FALSE;
  jmp_buf			JpegJumpBuffer;
  ILimage *   image;
	j_decompress_ptr JpegDecompress;
} JpegContext;

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */
  JOCTET * buffer;						/* start of buffer */
  boolean start_of_file;			/* have we gotten any data yet? */
  SIO *io;
} iread_mgr;

typedef struct
{
	struct jpeg_destination_mgr		pub;
	JOCTET							*buffer;
	ILboolean						bah; // humbug
	SIO *               io;
} iwrite_mgr;

#include "pack_pop.h"

typedef iwrite_mgr *iwrite_ptr;
typedef iread_mgr * iread_ptr;

#define INPUT_BUF_SIZE  4096  // choose an efficiently iread'able size
#define OUTPUT_BUF_SIZE 4096

static ILboolean iLoadFromJpegStruct(JpegContext *);

// Internal function used to get the .jpg header from the current file.
static ILuint iGetJpgHead(SIO* io, ILubyte *Header) {
	return SIOread(io, Header, 1, 2);
}

// Internal function used to check if the HEADER is a valid .Jpg header.
static ILboolean iCheckJpg(ILubyte Header[2]) {
	if (Header[0] != 0xFF || Header[1] != 0xD8)
		return IL_FALSE;

	return IL_TRUE;
}

// Internal function to get the header and check it.
static ILboolean iIsValidJpeg(SIO* io) {
	ILubyte Head[2];

	ILuint Start = SIOtell(io);
	ILuint read  = iGetJpgHead(io, Head);
	SIOseek(io, Start, IL_SEEK_SET);  // Go ahead and restore to previous state

	return read == 2 && iCheckJpg(Head);
}

static ILboolean iJpegSeekToExif(SIO *io) {
	static ILubyte SOI [2] = { 0xFF, 0xD8 };
	static ILubyte APP1[2] = { 0xFF, 0xE1 };

	ILubyte marker[2];
  if (SIOread(io, marker, 1, 2) != 2) return IL_FALSE;
  if (memcmp(marker, SOI, 2)) return IL_FALSE;

  do {
  	ILushort Size = 0;

  	if (SIOread(io, marker, 1, 2) != 2) return IL_FALSE;
  	if (SIOread(io, &Size, 1, 2) != 2) return IL_FALSE;

  	BigUShort(&Size);

  	if (memcmp(marker, APP1, 2)) {
  		SIOseek(io, Size-2, IL_SEEK_CUR);
  	} else {
  		ILuint pos = SIOtell(io);
  		ILubyte hdr[8];

	  	if (SIOread(io, hdr, 1, 8) != 8) return IL_FALSE;
	  	if (memcmp(hdr, "Exif\0\0MM", 8)) {
	  		SIOseek(io, pos + Size - 2, IL_SEEK_SET);
	  	} else {
	  		SIOseek(io, -2, IL_SEEK_CUR);
	  		return IL_TRUE;
	  	}
  	}
  } while (!SIOeof(io));
  return IL_FALSE;
}

// Overrides libjpeg's stupid error/warning handlers. =P
static void ExitErrorHandle (struct jpeg_common_struct *JpegInfo) NORETURN;
static void ExitErrorHandle (struct jpeg_common_struct *JpegInfo) {
	JpegContext *ctx = (JpegContext*)JpegInfo->client_data;
	ctx->jpgErrorOccured = IL_TRUE;
	if (JpegInfo->err) {
		iTrace("**** JPEG lib error: %s", JpegInfo->err->jpeg_message_table[JpegInfo->err->last_jpeg_message]);
	}
	iSetError(IL_LIB_JPEG_ERROR);
	longjmp( ctx->JpegJumpBuffer, 1 );
}

static void OutputMsg(struct jpeg_common_struct *JpegInfo) {
	if (JpegInfo->err) {
		iTrace("---- JPEG lib message: %s", JpegInfo->err->jpeg_message_table[JpegInfo->err->last_jpeg_message]);
	}
}

static void
init_source (j_decompress_ptr cinfo) {
	iread_ptr src = (iread_ptr) cinfo->src;
	src->start_of_file = TRUE;
}

static boolean
fill_input_buffer (j_decompress_ptr cinfo) {
	iread_ptr 		src 		= (iread_ptr) cinfo->src;
	JpegContext *	ctx 		= (JpegContext*)cinfo->client_data;
	ILuint 				nbytes 	= SIOread(src->io, src->buffer, 1, INPUT_BUF_SIZE);

	if (!nbytes) {
		if (src->start_of_file) {  
		  // Treat empty input file as fatal error
			//ERREXIT(cinfo, JERR_INPUT_EMPTY);
			ctx->jpgErrorOccured = IL_TRUE;
		}
		//WARNMS(cinfo, JWRN_JPEG_EOF);
		// Insert a fake EOI marker
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
		return IL_FALSE;
	}

	if (nbytes < INPUT_BUF_SIZE) {
		ilGetError();  // Gets rid of the IL_FILE_READ_ERROR.
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = IL_FALSE;

	return IL_TRUE;
}


static void skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
	iread_ptr src = (iread_ptr) cinfo->src;

	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}


static void term_source (j_decompress_ptr cinfo) {
	(void)cinfo;
	// no work necessary here
}


static void devil_jpeg_read_init (SIO *io, j_decompress_ptr cinfo) {
	iread_ptr src;

	if ( cinfo->src == NULL ) {  // first time for this JPEG object?
		cinfo->src = (struct jpeg_source_mgr *)
					 (*cinfo->mem->alloc_small)( (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(iread_mgr) );
		src = (iread_ptr) cinfo->src;
		src->buffer = (JOCTET *)
					  (*cinfo->mem->alloc_small)( (j_common_ptr)cinfo, JPOOL_PERMANENT,
												  INPUT_BUF_SIZE * sizeof(JOCTET) );
	}

	src = (iread_ptr) cinfo->src;
	src->io = io;
	src->pub.init_source 				= init_source;
	src->pub.fill_input_buffer 	= fill_input_buffer;
	src->pub.skip_input_data 		= skip_input_data;
	src->pub.resync_to_restart 	= jpeg_resync_to_restart;  // use default method
	src->pub.term_source 				= term_source;
	src->pub.bytes_in_buffer 		= 0;  // forces fill_input_buffer on first read
	src->pub.next_input_byte 		= NULL;  // until buffer loaded
}

// Internal function used to load the jpeg. must not be static, used by other loaders
ILboolean iLoadJpegInternal(ILimage* image) {
	struct jpeg_error_mgr					Error;
	struct jpeg_decompress_struct	JpegInfo;
	ILboolean											result;
	ILuint 												Start;
	JpegContext                   ctx;
	ILint                         setjmpResult;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ctx.jpgErrorOccured = IL_FALSE;
	ctx.image           = image;
	ctx.JpegDecompress  = &JpegInfo;

	Start = SIOtell(&image->io);

	JpegInfo.err 					= jpeg_std_error(&Error);		// init standard error handlers
	Error.error_exit 			= ExitErrorHandle;			  	// add our exit handler
	Error.output_message 	= OutputMsg;

	setjmpResult = setjmp(ctx.JpegJumpBuffer);
	if (setjmpResult == 0) {
		jpeg_create_decompress(&JpegInfo);
		JpegInfo.do_block_smoothing 	= IL_TRUE;
		JpegInfo.do_fancy_upsampling 	= IL_TRUE;
		JpegInfo.client_data        	= &ctx;

		devil_jpeg_read_init(&image->io, &JpegInfo);
		jpeg_read_header(&JpegInfo, IL_TRUE);

		result = iLoadFromJpegStruct(&ctx);

		jpeg_finish_decompress(&JpegInfo);
		jpeg_destroy_decompress(&JpegInfo);
	}	else {
		result = IL_FALSE;
		jpeg_destroy_decompress(&JpegInfo);
	}

	if (result) {
		ILuint End = SIOtell(&image->io);
		SIOseek(&image->io, Start, IL_SEEK_SET);
		if (iJpegSeekToExif(&image->io)) {
			iTrace("Exif header at %08x", SIOtell(&image->io));
			iExifLoad(image);
		} else {
			iTrace("Exif header not found");
		}
		SIOseek(&image->io, End, IL_SEEK_SET);
	}

	return result;
}

static void init_destination(j_compress_ptr cinfo) {
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	dest->buffer = (JOCTET *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

static boolean empty_output_buffer (j_compress_ptr cinfo) {
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	SIOwrite(dest->io, dest->buffer, 1, OUTPUT_BUF_SIZE);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return IL_TRUE;
}

static void term_destination (j_compress_ptr cinfo) {
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	SIOwrite(dest->io, dest->buffer, 1, OUTPUT_BUF_SIZE - (ILuint)dest->pub.free_in_buffer);
}

static void devil_jpeg_write_init(j_compress_ptr cinfo, SIO *io) {
	iwrite_ptr dest;

	if (cinfo->dest == NULL) {	// first time for this JPEG object?
		cinfo->dest = (struct jpeg_destination_mgr *)
		  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
					  sizeof(iwrite_mgr));
		dest = (iwrite_ptr)cinfo->dest;
	}

	dest = (iwrite_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->io = io;
}

// Internal function used to save the Jpeg.
static ILboolean iSaveJpegInternal(ILimage* image) {
	struct		jpeg_compress_struct JpegInfo;
	struct		jpeg_error_mgr Error;
	JSAMPROW	row_pointer[1];
	ILimage		*TempImage;
	ILubyte		*TempData;
	//ILuint APP1Pos, APP1Size;
	SIO *     io, iobak;
	ILuint    exifSize;
	ILubyte * exifData = NULL;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	io = &image->io;
	iobak = *io;

	exifSize = iDetermineSize(image, IL_EXIF) + 6;
	exifData = (ILubyte*)ialloc(exifSize);
	memcpy(exifData, "Exif\0\0", 6);
	image->io.handle = NULL;
	iSetOutputLump(image, exifData + 6, exifSize - 6);
	iExifSave(image);
	*io = iobak;

	if ((image->Format != IL_RGB && image->Format != IL_LUMINANCE) || image->Bpc != 1) {
		TempImage = iConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			ifree(exifData);
			return IL_FALSE;
		}
	}
	else {
		TempImage = image;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != image)
				iCloseImage(TempImage);
			ifree(exifData);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}


	JpegInfo.err = jpeg_std_error(&Error);
	Error.trace_level = 10;

	// Now we can initialize the JPEG compression object.
	jpeg_create_compress(&JpegInfo);

	//jpeg_stdio_dest(&JpegInfo, JpegFile);
	devil_jpeg_write_init(&JpegInfo, io);

	JpegInfo.image_width = TempImage->Width;  // image width and height, in pixels
	JpegInfo.image_height = TempImage->Height;
	JpegInfo.input_components = TempImage->Bpp;  // # of color components per pixel

	// John Villar's addition
	if (TempImage->Bpp == 1)
		JpegInfo.in_color_space = JCS_GRAYSCALE;
	else
		JpegInfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&JpegInfo);

	JpegInfo.write_JFIF_header = TRUE;

	// Set the quality output
	jpeg_set_quality(&JpegInfo, iGetInt(IL_JPG_QUALITY), IL_TRUE);
	// Sets progressive saving here
	if (iIsEnabled(IL_JPG_PROGRESSIVE))
		jpeg_simple_progression(&JpegInfo);

	jpeg_start_compress(&JpegInfo, IL_TRUE);

	// write exif

	jpeg_write_marker(&JpegInfo, JPEG_APP0+1, exifData, exifSize);

	ifree(exifData); exifData = NULL;

	//row_stride = image_width * 3;	// JSAMPLEs per row in image_buffer

	while (JpegInfo.next_scanline < JpegInfo.image_height) {
		// jpeg_write_scanlines expects an array of pointers to scanlines.
		// Here the array is only one element long, but you could pass
		// more than one scanline at a time if that's more convenient.
		row_pointer[0] = &TempData[JpegInfo.next_scanline * TempImage->Bps];
		(void) jpeg_write_scanlines(&JpegInfo, row_pointer, 1);
	}

	// Step 6: Finish compression
	jpeg_finish_compress(&JpegInfo);

	// Step 7: release JPEG compression object

	// This is an important step since it will release a good deal of memory.
	jpeg_destroy_compress(&JpegInfo);

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != image)
		iCloseImage(TempImage);

	ifree(exifData);
	return IL_TRUE;
}

static ILboolean iLoadFromJpegStruct(JpegContext *ctx) {
	// sam. void (*errorHandler)(j_common_ptr);
	ILubyte	*TempPtr[1];
	ILuint	Returned;
	ILimage* image = ctx->image;
	j_decompress_ptr JpegInfo = ctx->JpegDecompress;

	//added on 2003-08-31 as explained in sf bug 596793
	ctx->jpgErrorOccured = IL_FALSE;

	// sam. errorHandler = JpegInfo->err->error_exit;
	// sam. JpegInfo->err->error_exit = ExitErrorHandle;
	jpeg_start_decompress((j_decompress_ptr)JpegInfo);

	if (!iTexImage(image, JpegInfo->output_width, JpegInfo->output_height, 1, (ILubyte)JpegInfo->output_components, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (image->Bpp)	{
		case 1:			image->Format = IL_LUMINANCE; break;
		case 3:			image->Format = IL_RGB;  			break;
		case 4:			image->Format = IL_RGBA;			break;
		default:
			//@TODO: Anyway to get here?  Need to error out or something...
			break;
	}

	TempPtr[0] = image->Data;
	while (JpegInfo->output_scanline < JpegInfo->output_height) {
		Returned = jpeg_read_scanlines(JpegInfo, TempPtr, 1);  // anyway to make it read all at once?
		TempPtr[0] += image->Bps;
		if (Returned == 0)
			break;
	}

	if (ctx->jpgErrorOccured)
		return IL_FALSE;

	return IL_TRUE;
}

static ILconst_string iFormatExtsJPG[] = { 
  IL_TEXT("jfif"), 
  IL_TEXT("jif"), 
  IL_TEXT("jpe"), 
  IL_TEXT("jpeg"), 
  IL_TEXT("jpg"), 
  NULL 
};

ILformat iFormatJPG = { 
  /* .Validate = */ iIsValidJpeg, 
  /* .Load     = */ iLoadJpegInternal, 
  /* .Save     = */ iSaveJpegInternal, 
  /* .Exts     = */ iFormatExtsJPG
};

#endif//IL_NO_JPG
