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

// FIXME: ijl based implementation broken

#include "il_internal.h"

#ifndef IL_NO_JPG

ILboolean iExifLoad(ILimage *Image);
ILboolean iExifSave(ILimage *Image);

	#ifndef IL_USE_IJL
		#ifdef RGB_RED
			#undef RGB_RED
			#undef RGB_GREEN
			#undef RGB_BLUE
		#endif
		#define RGB_RED		0
		#define RGB_GREEN	1
		#define RGB_BLUE	2

		#if defined(_MSC_VER)
			#pragma warning(push)
			#pragma warning(disable : 4005)  // Redefinitions in
			#pragma warning(disable : 4142)  //  jmorecfg.h
		#endif

		#include "jpeglib.h"

		#if JPEG_LIB_VERSION < 62
			#warning DevIL was designed with libjpeg 6b or higher in mind.  Consider upgrading at www.ijg.org
		#endif
	#else
		#include <ijl.h>
		#include <limits.h>
	#endif

#include "il_jpeg.h"
#include "il_manip.h"
#include <setjmp.h>


#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifdef IL_USE_IJL
			//pragma comment(lib, "ijl15.lib")
		#else
			#ifndef _DEBUG
				#pragma comment(lib, "libjpeg.lib")
			#else
				#pragma comment(lib, "libjpeg-d.lib")
			#endif
		#endif//IL_USE_IJL
	#endif
#endif

// Global variables
static ILboolean 	jpgErrorOccured = IL_FALSE;
static jmp_buf		JpegJumpBuffer;

// define a protype of ilLoadFromJpegStruct
ILboolean ilLoadFromJpegStruct(ILimage* image, void *_JpegInfo);

// Internal function used to get the .jpg header from the current file.
ILint iGetJpgHead(SIO* io, ILubyte *Header) {
	return SIOread(io, Header, 1, 2);
}

// Internal function used to check if the HEADER is a valid .Jpg header.
ILboolean iCheckJpg(ILubyte Header[2]) {
	if (Header[0] != 0xFF || Header[1] != 0xD8)
		return IL_FALSE;

	return IL_TRUE;
}

// Internal function to get the header and check it.
static ILboolean iIsValidJpeg(SIO* io) {
	ILubyte Head[2];

	ILint read = iGetJpgHead(io, Head);
	io->seek(io->handle, -read, IL_SEEK_CUR);  // Go ahead and restore to previous state

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

#ifndef IL_USE_IJL // Use libjpeg instead of the IJL.

// Overrides libjpeg's stupid error/warning handlers. =P
void ExitErrorHandle (struct jpeg_common_struct *JpegInfo) {
	(void)JpegInfo;
	iSetError(IL_LIB_JPEG_ERROR);
	jpgErrorOccured = IL_TRUE;
	return;
}

void OutputMsg(struct jpeg_common_struct *JpegInfo) {
	(void)JpegInfo;
	// iTrace(...)
	return;
}

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */
  JOCTET * buffer;						/* start of buffer */
  boolean start_of_file;			/* have we gotten any data yet? */
  SIO *io;
} iread_mgr;

typedef iread_mgr * iread_ptr;

#define INPUT_BUF_SIZE  4096  // choose an efficiently iread'able size

METHODDEF(void)
init_source (j_decompress_ptr cinfo) {
	iread_ptr src = (iread_ptr) cinfo->src;
	src->start_of_file = TRUE;
}

METHODDEF(boolean) 
fill_input_buffer (j_decompress_ptr cinfo) {
	iread_ptr src = (iread_ptr) cinfo->src;
	ILint nbytes;

	nbytes = SIOread(src->io, src->buffer, 1, INPUT_BUF_SIZE);

	if (nbytes <= 0) {
		if (src->start_of_file) {  // Treat empty input file as fatal error
			//ERREXIT(cinfo, JERR_INPUT_EMPTY);
			jpgErrorOccured = IL_TRUE;
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


METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
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


METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
	(void)cinfo;
	
	// no work necessary here
}


GLOBAL(void)
devil_jpeg_read_init (SIO *io, j_decompress_ptr cinfo)
{
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


static void iJpegErrorExit( j_common_ptr cinfo )
{
	iSetError( IL_LIB_JPEG_ERROR );
	jpeg_destroy( cinfo );
	longjmp( JpegJumpBuffer, 1 );
}

// Internal function used to load the jpeg. must not be static, used by other loaders
ILboolean iLoadJpegInternal(ILimage* image)
{
	struct jpeg_error_mgr			Error;
	struct jpeg_decompress_struct	JpegInfo;
	ILboolean						result;
	ILuint Start;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Start = SIOtell(&image->io);

	JpegInfo.err = jpeg_std_error(&Error);		// init standard error handlers
	Error.error_exit = iJpegErrorExit;				// add our exit handler
	Error.output_message = OutputMsg;

	if ((result = setjmp(JpegJumpBuffer) == 0) != IL_FALSE) {
		jpeg_create_decompress(&JpegInfo);
		JpegInfo.do_block_smoothing = IL_TRUE;
		JpegInfo.do_fancy_upsampling = IL_TRUE;

		//jpeg_stdio_src(&JpegInfo, iGetFile());

		devil_jpeg_read_init(&image->io, &JpegInfo);
		jpeg_read_header(&JpegInfo, IL_TRUE);

		result = ilLoadFromJpegStruct(image, &JpegInfo);

		jpeg_finish_decompress(&JpegInfo);
		jpeg_destroy_decompress(&JpegInfo);

	}
	else
	{
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

	//return IL_TRUE;  // No need to call it again (called first in ilLoadFromJpegStruct).
	return result;
}



typedef struct
{
	struct jpeg_destination_mgr		pub;
	JOCTET							*buffer;
	ILboolean						bah; // humbug
	SIO *               io;
} iwrite_mgr;

typedef iwrite_mgr *iwrite_ptr;

#define OUTPUT_BUF_SIZE 4096


METHODDEF(void)
init_destination(j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	dest->buffer = (JOCTET *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return;
}

METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	SIOwrite(dest->io, dest->buffer, 1, OUTPUT_BUF_SIZE);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return IL_TRUE;
}

METHODDEF(void)
term_destination (j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	SIOwrite(dest->io, dest->buffer, 1, OUTPUT_BUF_SIZE - (ILuint)dest->pub.free_in_buffer);
	return;
}


GLOBAL(void)
devil_jpeg_write_init(j_compress_ptr cinfo, SIO *io)
{
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
	return;
}


// Internal function used to save the Jpeg.
ILboolean iSaveJpegInternal(ILimage* image)
{
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
	devil_jpeg_write_init(&JpegInfo, &TempImage->io);

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
	if (ilGetBoolean(IL_JPG_PROGRESSIVE))
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
/*
	SIOseek(io, -2, IL_SEEK_CUR);
	APP1Pos = SIOtell(io);

	// write dummy APP1
	SIOputc(io, 0xFF);
	SIOputc(io, 0xE1);
	SIOputc(io, 0);
	SIOputc(io, 0);
	SIOputs(io, "Exif");
	SIOputc(io, 0);
	SIOputc(io, 0);

	iExifSave(image);

	APP1Size = SIOtell(io) - APP1Pos;

	// write EOI
	SIOputc(io, 0xFF);
	SIOputc(io, 0xD9);

	// go back to APP1 and save size
	SIOseek(io, APP1Pos, IL_SEEK_SET);
	SIOputc(io, 0xFF);
	SIOputc(io, 0xE1);
	SIOputc(io, (APP1Size >> 8) & 0xFF);
	SIOputc(io,  APP1Size       & 0xFF);

	SIOseek(io, 0, IL_SEEK_END);
*/
	ifree(exifData);
	return IL_TRUE;
}



#else // Use the IJL instead of libjpeg.



//! Reads a jpeg file
ILboolean ilLoadJpeg(ILconst_string FileName)
{
	if (!iFileExists(FileName)) {
		iSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}
	return iLoadJpegInternal(FileName, NULL, 0);
}


// Reads from a memory "lump" containing a jpeg
ILboolean ilLoadJpegL(void *Lump, ILuint Size)
{
	return iLoadJpegInternal(NULL, Lump, Size);
}


// Internal function used to load the jpeg.
ILboolean iLoadJpegInternal(ILstring FileName, void *Lump, ILuint Size)
{
    JPEG_CORE_PROPERTIES Image;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (ijlInit(&Image) != IJL_OK) {
		iSetError(IL_LIB_JPEG_ERROR);
		return IL_FALSE;
	}

	if (FileName != NULL) {
		Image.JPGFile = FileName;
		if (ijlRead(&Image, IJL_JFILE_READPARAMS) != IJL_OK) {
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		Image.JPGBytes = Lump;
		Image.JPGSizeBytes = Size > 0 ? Size : UINT_MAX;
		if (ijlRead(&Image, IJL_JBUFF_READPARAMS) != IJL_OK) {
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	switch (Image.JPGChannels)
	{
		case 1:
			Image.JPGColor		= IJL_G;
			Image.DIBChannels	= 1;
			Image.DIBColor		= IJL_G;
			image->Format	= IL_LUMINANCE;
			break;

		case 3:
			Image.JPGColor		= IJL_YCBCR;
			Image.DIBChannels	= 3;
			Image.DIBColor		= IJL_RGB;
			image->Format	= IL_RGB;
			break;

        case 4:
			Image.JPGColor		= IJL_YCBCRA_FPX;
			Image.DIBChannels	= 4;
			Image.DIBColor		= IJL_RGBA_FPX;
			image->Format	= IL_RGBA;
			break;

        default:
			// This catches everything else, but no
			// color twist will be performed by the IJL.
			/*Image.DIBColor = (IJL_COLOR)IJL_OTHER;
			Image.JPGColor = (IJL_COLOR)IJL_OTHER;
			Image.DIBChannels = Image.JPGChannels;
			break;*/
			ijlFree(&Image);
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
	}

	if (!iTexImage(Image, Image.JPGWidth, Image.JPGHeight, 1, (ILubyte)Image.DIBChannels, image->Format, IL_UNSIGNED_BYTE, NULL)) {
		ijlFree(&Image);
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	Image.DIBWidth		= Image.JPGWidth;
	Image.DIBHeight		= Image.JPGHeight;
	Image.DIBPadBytes	= 0;
	Image.DIBBytes		= image->Data;

	if (FileName != NULL) {
		if (ijlRead(&Image, IJL_JFILE_READWHOLEIMAGE) != IJL_OK) {
			ijlFree(&Image);
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		if (ijlRead(&Image, IJL_JBUFF_READWHOLEIMAGE) != IJL_OK) {
			ijlFree(&Image);
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	ijlFree(&Image);
	return IL_TRUE;
}


//! Writes a Jpeg file
ILboolean ilSaveJpeg(ILconst_string FileName)
{
	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			iSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	return iSaveJpegInternal(FileName, NULL, 0);
}


//! Writes a Jpeg to a memory "lump"
ILboolean ilSaveJpegL(void *Lump, ILuint Size)
{
	return iSaveJpegInternal(NULL, Lump, Size);
}


// Internal function used to save the Jpeg.
ILboolean iSaveJpegInternal(ILstring FileName, void *Lump, ILuint Size)
{
	JPEG_CORE_PROPERTIES	Image;
	ILuint	Quality;
	ILimage	*TempImage;
	ILubyte	*TempData;

	imemclear(&Image, sizeof(JPEG_CORE_PROPERTIES));

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (FileName == NULL && Lump == NULL) {
		iSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Quality = 85;  // Not sure how low we should dare go...
	else
		Quality = 99;

	if (ijlInit(&Image) != IJL_OK) {
		iSetError(IL_LIB_JPEG_ERROR);
		return IL_FALSE;
	}

	if ((image->Format != IL_RGB && image->Format != IL_RGBA && image->Format != IL_LUMINANCE)
		|| image->Bpc != 1) {
		if (image->Format == IL_BGRA)
			Temp = iConvertImage(image, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			Temp = iConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
		if (Temp == NULL) {
			return IL_FALSE;
		}
	}
	else {
		Temp = image;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != image)
				iCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	// Setup DIB
	Image.DIBWidth		= TempImage->Width;
	Image.DIBHeight		= TempImage->Height;
	Image.DIBChannels	= TempImage->Bpp;
	Image.DIBBytes		= TempData;
	Image.DIBPadBytes	= 0;

	// Setup JPEG
	Image.JPGWidth		= TempImage->Width;
	Image.JPGHeight		= TempImage->Height;
	Image.JPGChannels	= TempImage->Bpp;

	switch (Temp->Bpp)
	{
		case 1:
			Image.DIBColor			= IJL_G;
			Image.JPGColor			= IJL_G;
			Image.JPGSubsampling	= IJL_NONE;
			break;
		case 3:
			Image.DIBColor			= IJL_RGB;
			Image.JPGColor			= IJL_YCBCR;
			Image.JPGSubsampling	= IJL_411;
			break;
		case 4:
			Image.DIBColor			= IJL_RGBA_FPX;
			Image.JPGColor			= IJL_YCBCRA_FPX;
			Image.JPGSubsampling	= IJL_4114;
			break;
	}

	if (FileName != NULL) {
		Image.JPGFile = FileName;
		if (ijlWrite(&Image, IJL_JFILE_WRITEWHOLEIMAGE) != IJL_OK) {
			if (TempImage != image)
				iCloseImage(TempImage);
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		Image.JPGBytes = Lump;
		Image.JPGSizeBytes = Size;
		if (ijlWrite(&Image, IJL_JBUFF_WRITEWHOLEIMAGE) != IJL_OK) {
			if (TempImage != image)
				iCloseImage(TempImage);
			iSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	ijlFree(&Image);

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (Temp != image)
		iCloseImage(Temp);

	return IL_TRUE;
}

#endif//IL_USE_IJL


// Access point for applications wishing to use the jpeg library directly in
// conjunction with DevIL.
//
// The decompressor must be set up with an input source and all desired parameters
// this function is called. The caller must call jpeg_finish_decompress because
// the caller may still need decompressor after calling this for e.g. examining
// saved markers.
ILboolean ilLoadFromJpegStruct(ILimage* image, void *_JpegInfo)
{
#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	// sam. void (*errorHandler)(j_common_ptr);
	ILubyte	*TempPtr[1];
	ILuint	Returned;
	j_decompress_ptr JpegInfo = (j_decompress_ptr)_JpegInfo;

	//added on 2003-08-31 as explained in sf bug 596793
	jpgErrorOccured = IL_FALSE;

	// sam. errorHandler = JpegInfo->err->error_exit;
	// sam. JpegInfo->err->error_exit = ExitErrorHandle;
	jpeg_start_decompress((j_decompress_ptr)JpegInfo);

	if (!iTexImage(image, JpegInfo->output_width, JpegInfo->output_height, 1, (ILubyte)JpegInfo->output_components, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (image->Bpp)
	{
		case 1:
			image->Format = IL_LUMINANCE;
			break;
		case 3:
			image->Format = IL_RGB;
			break;
		case 4:
			image->Format = IL_RGBA;
			break;
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

	// sam. JpegInfo->err->error_exit = errorHandler;

	if (jpgErrorOccured)
		return IL_FALSE;

	return IL_TRUE;
#endif
#endif
	return IL_FALSE;
}



// Access point for applications wishing to use the jpeg library directly in
// conjunction with DevIL.
//
// The caller must set up the desired parameters by e.g. calling
// jpeg_set_defaults and overriding the parameters the caller wishes
// to change, such as quality, before calling this function. The caller
// is also responsible for calling jpeg_finish_compress in case the
// caller still needs to compressor for something.
// 
ILboolean ilSaveFromJpegStruct(ILimage* image, void *_JpegInfo)
{
#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	//void (*errorHandler)();
	JSAMPROW	row_pointer[1];
	ILimage		*TempImage;
	ILubyte		*TempData;
	j_compress_ptr JpegInfo = (j_compress_ptr)_JpegInfo;

	if (image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//added on 2003-08-31 as explained in SF bug 596793
	jpgErrorOccured = IL_FALSE;

	//errorHandler = JpegInfo->err->error_exit;
	JpegInfo->err->error_exit = ExitErrorHandle;


	if ((image->Format != IL_RGB && image->Format != IL_LUMINANCE) || image->Bpc != 1) {
		TempImage = iConvertImage(image, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
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
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	JpegInfo->image_width = TempImage->Width;  // image width and height, in pixels
	JpegInfo->image_height = TempImage->Height;
	JpegInfo->input_components = TempImage->Bpp;  // # of color components per pixel

	jpeg_start_compress(JpegInfo, IL_TRUE);

	//row_stride = image_width * 3;	// JSAMPLEs per row in image_buffer

	while (JpegInfo->next_scanline < JpegInfo->image_height) {
		// jpeg_write_scanlines expects an array of pointers to scanlines.
		// Here the array is only one element long, but you could pass
		// more than one scanline at a time if that's more convenient.
		row_pointer[0] = &TempData[JpegInfo->next_scanline * TempImage->Bps];
		(void) jpeg_write_scanlines(JpegInfo, row_pointer, 1);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != image)
		iCloseImage(TempImage);

	return (!jpgErrorOccured);
#endif//IL_USE_IJL
#endif//IL_NO_JPG
	return IL_FALSE;
}


#if defined(_MSC_VER)
	#pragma warning(pop)
	//#pragma warning(disable : 4756)  // Disables 'named type definition in parentheses' warning
#endif

ILconst_string iFormatExtsJPG[] = { 
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
