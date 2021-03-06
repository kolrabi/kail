//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_mng.c
//
// Description: Multiple Network Graphics (.mng) functions
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_MNG
#define MNG_SUPPORT_READ
#define MNG_SUPPORT_WRITE
#define MNG_SUPPORT_DISPLAY

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 4142)  // Redefinition in jmorecfg.h
#endif

#include <libmng.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


static ILboolean iIsValidMng(SIO* io) {
	ILubyte mngSig[8];
	ILuint Start = SIOtell(io);
	ILuint read = SIOread(io, mngSig, 1, sizeof(mngSig));
	SIOseek(io, Start, IL_SEEK_SET);

	if (read == 8
	&&  mngSig[0] == 0x8A 
	&&  mngSig[1] == 0x4D
	&&  mngSig[2] == 0x4E
	&&  mngSig[3] == 0x47
	&&  mngSig[4] == 0x0D
	&&  mngSig[5] == 0x0A
	&&  mngSig[6] == 0x1A
	&&  mngSig[7] == 0x0A)
	{
		return IL_TRUE;
	} else {
		return IL_FALSE;
	}
}

//---------------------------------------------------------------------------------------------
// memory allocation; data must be zeroed
//---------------------------------------------------------------------------------------------
static mng_ptr MNG_DECL mymngalloc(mng_size_t size)
{
	return (mng_ptr)icalloc(1, size);
}


//---------------------------------------------------------------------------------------------
// memory deallocation
//---------------------------------------------------------------------------------------------
static void MNG_DECL mymngfree(mng_ptr p, mng_size_t size)
{
	(void)size;
	ifree(p);
}


//---------------------------------------------------------------------------------------------
// Stream open:
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngopenstream(mng_handle mng)
{
	(void)mng;
	return MNG_TRUE;
}


//---------------------------------------------------------------------------------------------
// Stream open for Writing:
//---------------------------------------------------------------------------------------------
/* static mng_bool MNG_DECL mymngopenstreamwrite(mng_handle mng)
{
	(void)mng;
	return MNG_TRUE;
} */


//---------------------------------------------------------------------------------------------
// Stream close:
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngclosestream(mng_handle mng)
{
	(void)mng;
	return MNG_TRUE; // We close the file ourself, mng_cleanup doesnt seem to do it...
}


//---------------------------------------------------------------------------------------------
// feed data to the decoder
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngreadstream(mng_handle mng, mng_ptr buffer, mng_size_t size, mng_uint32 *bytesread)
{
	// read the requested amount of data from the file
	ILimage *Image = (ILimage*)mng_get_userdata(mng);
	*bytesread = SIOread(&Image->io, buffer, 1, (ILuint)size);

	return MNG_TRUE;
}


//---------------------------------------------------------------------------------------------
// callback for writing data
//---------------------------------------------------------------------------------------------
/* static mng_bool MNG_DECL mymngwritedata(mng_handle mng, mng_ptr buffer, mng_size_t size, mng_uint32 *byteswritten)
{
	ILimage *Image = (ILimage*)mng_get_userdata(mng);
	*byteswritten = SIOwrite(&Image->io, buffer, 1, (ILuint)size);

	if (*byteswritten < size) {
		iSetError(IL_FILE_WRITE_ERROR);
		return MNG_FALSE;
	}

	return MNG_TRUE;
}*/


//---------------------------------------------------------------------------------------------
// the header's been read. set up the display stuff
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngprocessheader(mng_handle mng, mng_uint32 width, mng_uint32 height)
{
	ILimage *Image = (ILimage*)mng_get_userdata(mng);
	ILuint	AlphaDepth;

	AlphaDepth = mng_get_alphadepth(mng);

	if (AlphaDepth == 0) {
		iTexImage(Image, width, height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL);
		Image->Origin = IL_ORIGIN_LOWER_LEFT;
		mng_set_canvasstyle(mng, MNG_CANVAS_BGR8);
	}
	else {  // Use alpha channel
		iTexImage(Image, width, height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
		Image->Origin = IL_ORIGIN_LOWER_LEFT;
		mng_set_canvasstyle(mng, MNG_CANVAS_BGRA8);
	}

	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	return MNG_TRUE;
}


//---------------------------------------------------------------------------------------------
// return a row pointer for the decoder to fill
//---------------------------------------------------------------------------------------------
static mng_ptr MNG_DECL mymnggetcanvasline(mng_handle mng, mng_uint32 line)
{
	ILimage *Image = (ILimage*)mng_get_userdata(mng);
	return (mng_ptr)(Image->Data + Image->Bps * line);
}


//---------------------------------------------------------------------------------------------
// timer
//---------------------------------------------------------------------------------------------
static mng_uint32 MNG_DECL mymnggetticks(mng_handle mng)
{
	(void)mng;
	return 0;
}


//---------------------------------------------------------------------------------------------
// Refresh:
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngrefresh(mng_handle mng, mng_uint32 x, mng_uint32 y, mng_uint32 w, mng_uint32 h)
{
	(void)mng;
	(void)x;
  (void)y;
  (void)w;
  (void)h;
	return MNG_TRUE;
}


//---------------------------------------------------------------------------------------------
// interframe delay callback
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngsettimer(mng_handle mng, mng_uint32 msecs)
{
	(void)mng;
	(void)msecs;
	return MNG_TRUE;
}



// Make this do something different soon!

//---------------------------------------------------------------------------------------------
// Error Callback;
//---------------------------------------------------------------------------------------------
static mng_bool MNG_DECL mymngerror(
	mng_handle mng, mng_int32 code, mng_int8 severity,
	mng_chunkid chunktype, mng_uint32 chunkseq,
	mng_int32 extra1, mng_int32 extra2, mng_pchar text
	)
{
	(void)mng;
	(void)code;
	(void)severity;
	(void)chunktype;
	(void)chunkseq;
	(void)extra1;
	(void)extra2;

	iTrace("**** MNG Error: %s", text);
	// error occured;
	return MNG_FALSE;
}


static ILboolean iLoadMngInternal(ILimage *Image)
{
	mng_handle mng;

	if (Image == NULL) {
		iSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	mng = mng_initialize(MNG_NULL, mymngalloc, mymngfree, MNG_NULL);
	if (mng == MNG_NULL) {
		iSetError(IL_LIB_MNG_ERROR);
		return IL_FALSE;
	}

	// If .mng background is available, use it.
	mng_set_usebkgd(mng, MNG_TRUE);

	// Set the callbacks.
	mng_setcb_errorproc 		(mng, mymngerror);
  mng_setcb_openstream 		(mng, mymngopenstream);
  mng_setcb_closestream		(mng, mymngclosestream);
  mng_setcb_readdata			(mng, (mng_readdata)mymngreadstream);
	mng_setcb_gettickcount	(mng, mymnggetticks);
	mng_setcb_settimer			(mng, mymngsettimer);
	mng_setcb_processheader	(mng, mymngprocessheader);
	mng_setcb_getcanvasline (mng, mymnggetcanvasline);
	mng_setcb_refresh 			(mng, mymngrefresh);

	mng_set_userdata 				(mng, Image);

	mng_read(mng);
	mng_display(mng);

	return IL_TRUE;
}


#if 0
// Internal function used to save the Mng.
static ILboolean iSaveMngInternal(ILimage *)
{
	//mng_handle mng;

	// Not working yet, so just error out.
	iSetError(IL_INVALID_EXTENSION);
	return IL_FALSE;

	//if (iCurImage == NULL) {
	//	iSetError(IL_ILLEGAL_OPERATION);
	//	return IL_FALSE;
	//}

	//mng = mng_initialize(MNG_NULL, mymngalloc, mymngfree, MNG_NULL);
	//if (mng == MNG_NULL) {
	//	iSetError(IL_LIB_MNG_ERROR);
	//	return IL_FALSE;
	//}

	//mng_setcb_openstream(mng, mymngopenstreamwrite);
	//mng_setcb_closestream(mng, mymngclosestream);
	//mng_setcb_writedata(mng, mymngwritedata);

	//// Write File:
 //  	mng_create(mng);

	//// Check return value.
	//mng_putchunk_mhdr(mng, iCurImage->Width, iCurImage->Height, 1000, 3, 1, 3, 0x0047);
	//mng_putchunk_basi(mng, iCurImage->Width, iCurImage->Height, 8, MNG_COLORTYPE_RGB, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 1);
	//mng_putchunk_iend(mng);
	//mng_putimgdata_ihdr(mng, iCurImage->Width, iCurImage->Height, MNG_COLORTYPE_RGB, 8, 0, 0, 0, 0, mymnggetcanvasline);

	//// Now write file:
	//mng_write(mng);

	//mng_cleanup(&mng);

	//return IL_TRUE;
}
#endif

static ILconst_string iFormatExtsMNG[] = { 
  IL_TEXT("mng"), 
  IL_TEXT("jng"),
  NULL 
};

ILformat iFormatMNG = { 
  /* .Validate = */ iIsValidMng, 
  /* .Load     = */ iLoadMngInternal, 
  /* .Save     = */ NULL, // FIXME: broken iSaveMngInternal, 
  /* .Exts     = */ iFormatExtsMNG
};

#endif//IL_NO_MNG
