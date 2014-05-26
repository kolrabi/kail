//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_iCurImage->io.c
//
// Description: Determines image types and loads/saves images
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#include "il_register.h"
#include "il_pal.h"
#include <string.h>

#include "il_formats.h"

// Returns a widened version of a string.
// Make sure to free this after it is used.  Code help from
//  https://buildsecurityin.us-cert.gov/daisy/bsi-rules/home/g1/769-BSI.html
#if defined(_UNICODE)
wchar_t *WideFromMultiByte(const char *Multi)
{
	ILint	Length;
	wchar_t	*Temp;

	Length = (ILint)mbstowcs(NULL, (const char*)Multi, 0) + 1; // note error return of -1 is possible
	if (Length == 0) {
		ilSetError(IL_INVALID_PARAM);
		return NULL;
	}
	if (Length > ULONG_MAX/sizeof(wchar_t)) {
		ilSetError(IL_INTERNAL_ERROR);
		return NULL;
	}
	Temp = (wchar_t*)ialloc(Length * sizeof(wchar_t));
	mbstowcs(Temp, (const char*)Multi, Length); 

	return Temp;
}
#endif


ILenum ILAPIENTRY ilTypeFromExt(ILconst_string FileName)
{
	ILenum		Type;
	ILstring	Ext;

	if (FileName == NULL || iStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_TYPE_UNKNOWN;
	}

	Ext = iGetExtension(FileName);
	//added 2003-08-31: fix sf bug 789535
	if (Ext == NULL) {
		return IL_TYPE_UNKNOWN;
	}

	else if (!iStrCmp(Ext, IL_TEXT("exr")))
		Type = IL_EXR;
	else if (!iStrCmp(Ext, IL_TEXT("wdp")) || !iStrCmp(Ext, IL_TEXT("hdp")))
		Type = IL_WDP;
	else
		iIdentifyFormatExt(Ext);

	return Type;
}


//changed 2003-09-17 to ILAPIENTRY
// uses globally set io functions (ilSetRead/ilSetReadF)
ILenum ILAPIENTRY ilDetermineType(ILconst_string FileName)
{
	ILenum Type = IL_TYPE_UNKNOWN;

	if (FileName != NULL) {
		// If we can open the file, determine file type from contents
		// This is more reliable than the file name extension 
		iSetFuncs(&iCurImage->io);
		iCurImage->io.handle = iCurImage->io.openReadOnly(FileName);

		if (iCurImage->io.handle != NULL) {
			Type = ilDetermineTypeFuncs();
			iCurImage->io.close(iCurImage->io.handle);
		}

		if (Type == IL_TYPE_UNKNOWN)
			Type = ilTypeFromExt(FileName);
	}

	return Type;
}

ILenum ILAPIENTRY ilDetermineTypeFuncs()
{
	// Read data only once, then check the contents once: this is quicker than 
	// loading and checking the data for every check that is run sequentially
	// The following code assumes that it is okay to identify formats even when they're not supported
	const int bufSize = 512;
	ILubyte buf[bufSize];
	ILint64 read = iCurImage->io.read(iCurImage->io.handle, buf, 1, bufSize);
	iCurImage->io.seek(iCurImage->io.handle, -read, SEEK_CUR);
	if (read < 16)
		return IL_TYPE_UNKNOWN;

	switch(buf[0]) {
		case 'A':
			if (buf[1] == 'H')
				return IL_HALO_PAL;
			break;
		case 'I':
			if (buf[1] == 'I') {
				if (buf[2] == 0xBC){
					return IL_WDP;
				}
			}
		case 'v':
			if (buf[1] == '/' && buf[2] == '1' && buf[3] == 1)
				#ifndef IL_NO_EXR
				if (iIsValidExr())
				#endif
					return IL_EXR;
			break;
	}

	return iIdentifyFormat(&iCurImage->io);
}

ILenum ILAPIENTRY ilDetermineTypeF(ILHANDLE File)
{
	iSetInputFile(iCurImage, File);
	return ilDetermineTypeFuncs();
}

ILenum ILAPIENTRY ilDetermineTypeL(const void *Lump, ILuint Size)
{
	iSetInputLump(iCurImage, Lump, Size);
	return ilDetermineTypeFuncs();
}

ILboolean ILAPIENTRY iIsValid(ILenum Type, SIO* io)
{
	if (io == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	switch (Type)
	{
		#ifndef IL_NO_EXR
		case IL_EXR:
			return ilIsValidExr(io);
		#endif
	}

	const ILformat *format = iGetFormat(Type);
	if (format) {
		return format->Validate && format->Validate(io);
	}

	ilSetError(IL_INVALID_ENUM);
	return IL_FALSE;
}

ILboolean ILAPIENTRY ilIsValid(ILenum Type, ILconst_string FileName)
{
	ILboolean result = IL_FALSE;

	if (FileName != NULL) {
		// If we can open the file, determine file type from contents
		// This is more reliable than the file name extension 
		iSetFuncs(&iCurImage->io);
		iCurImage->io.handle = iCurImage->io.openReadOnly(FileName);
		if (iCurImage->io.handle != NULL) {
			result = iIsValid(Type, &iCurImage->io);
		}
	}

	return result;
	ilSetError(IL_INVALID_ENUM);
	return IL_FALSE;
}


ILboolean ILAPIENTRY ilIsValidF(ILenum Type, ILHANDLE File)
{
	iSetInputFile(iCurImage, File);
	return iIsValid(Type, &iCurImage->io);
}


ILboolean ILAPIENTRY ilIsValidL(ILenum Type, void *Lump, ILuint Size)
{
	iSetInputLump(iCurImage, Lump, Size);
	return iIsValid(Type, &iCurImage->io);
}


//! Attempts to load an image from a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoad will try to determine the type of the file and load it.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename of the file to load.
	\return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
	       have been tried and failed.*/
ILboolean ILAPIENTRY ilLoad(ILenum Type, ILconst_string FileName)
{
	if (FileName == NULL || iStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	iSetFuncs(&iCurImage->io);
	iCurImage->io.handle = iCurImage->io.openReadOnly(FileName);
	if (iCurImage->io.handle != NULL) {
		ILboolean bRet = ilLoadFuncs(Type);
		iCurImage->io.close(iCurImage->io.handle);

		if (Type == IL_CUT) {
			// Attempt to load the palette
			size_t fnLen = iStrLen(FileName);
			if (fnLen > 4) {
				if (FileName[fnLen-4] == '.'
				&&  FileName[fnLen-3] == 'c'
				&&  FileName[fnLen-2] == 'u'
				&&  FileName[fnLen-1] == 't') {
					ILchar* palFN = (ILchar*) ialloc(sizeof(ILchar)*fnLen+2);
					iStrCpy(palFN, FileName);
					iStrCpy(&palFN[fnLen-3], IL_TEXT("pal"));
					iCurImage->io.handle = iCurImage->io.openReadOnly(palFN);
					if (iCurImage->io.handle != NULL) {
						ilLoadHaloPal(palFN);
						iCurImage->io.close(iCurImage->io.handle);
					}
					ifree(palFN);
				}
			}
		}

		return bRet;
	} else
		return IL_FALSE;
}


//! Attempts to load an image from a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadF will try to determine the type of the file and load it.
	\param File File stream to load from. The caller is responsible for closing the handle.
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILboolean ILAPIENTRY ilLoadF(ILenum Type, ILHANDLE File)
{
	if (File == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	iSetFuncs(&iCurImage->io);
	iCurImage->io.handle = File;
	iCurImage->io.seek(iCurImage->io.handle, 0, IL_SEEK_SET);
	if (Type == IL_TYPE_UNKNOWN)
		Type = ilDetermineTypeFuncs();

	ILboolean bRet = ilLoadFuncs2(iCurImage, Type);

	return bRet;
}

ILboolean ILAPIENTRY ilLoadFuncs2(ILimage* image, ILenum type)
{
	switch (type)
	{
		case IL_TYPE_UNKNOWN:
			return IL_FALSE;

		#ifndef IL_NO_EXR
		case IL_EXR:
			return iLoadExrInternal();
		#endif
	}

	const ILformat *format = iGetFormat(type);
	if (format) {
		return format->Load != NULL && format->Load(image);
	}

	ilSetError(IL_INVALID_ENUM);
	return IL_FALSE;
}

//! Attempts to load an image using the currently set IO functions. The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadFuncs fails.
	\param File File stream to load from.
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILboolean ILAPIENTRY ilLoadFuncs(ILenum type)
{
	iSetFuncs(&iCurImage->io);
	if (type == IL_TYPE_UNKNOWN)
		type = ilDetermineTypeFuncs();

	return ilLoadFuncs2(iCurImage, type); // FIXME: call ilFixImage here instead of in loaders
}

//! Attempts to load an image from a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
	IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
	IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
	IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
	IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
	If IL_TYPE_UNKNOWN is specified, ilLoadL will try to determine the type of the file and load it.
	\param Lump The buffer where the file data is located
	\param Size Size of the buffer
	\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILboolean ILAPIENTRY ilLoadL(ILenum Type, const void *Lump, ILuint Size)
{
	iTrace("**** %04x %p, %u", Type, Lump, Size);
	if (Lump == NULL || Size == 0) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	iSetInputLump(iCurImage, Lump, Size);
	return ilLoadFuncs2(iCurImage, Type);
}


//! Attempts to load an image from a file with various different methods before failing - very generic.
/*! The ilLoadImage function allows a general interface to the specific internal file-loading
	routines.  First, it finds the extension and checks to see if any user-registered functions
	(registered through ilRegisterLoad) match the extension. If nothing matches, it takes the
	extension and determines which function to call based on it. Lastly, it attempts to identify
	the image based on various image header verification functions, such as ilIsValidPngF.
	If all this checking fails, IL_FALSE is returned with no modification to the current bound image.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename of the file to load.
	\return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
	       have been tried and failed.*/
ILboolean ILAPIENTRY ilLoadImage(ILconst_string FileName)
{
	iSetFuncs(&iCurImage->io);

	ILenum type = ilDetermineType(FileName);

	if (type != IL_TYPE_UNKNOWN) {
		return ilLoad(type, FileName);
	} else {
		ilSetError(IL_INVALID_EXTENSION);
		return IL_FALSE;
	}
}


ILboolean ILAPIENTRY ilSaveFuncs2(ILimage* image, ILenum type)
{
	ILboolean bRet = IL_FALSE;

	iSetFuncs(&image->io);

	switch(type) {

	#ifndef IL_NO_EXR
	case IL_EXR:
		bRet = iSaveExrInternal(FileName);
		break;
	#endif

	// Check if we just want to save the palette.
	// @todo: must be ported to use iCurImage->io
/*	case IL_JASC_PAL:
		bRet = ilSavePal(FileName);
		break;*/
	default:
		break;
	}

	const ILformat *format = iGetFormat(type);
	if (format) {
		return format->Save != NULL && format->Save(image);
	}

	ilSetError(IL_INVALID_ENUM);

	// Try registered procedures
	// @todo: must be ported to use iCurImage->io
	/*if (iRegisterSave(FileName))
		return IL_TRUE;*/

	return bRet;
}

ILAPI ILboolean ILAPIENTRY ilSaveFuncs(ILenum type)
{
	return ilSaveFuncs2(iCurImage, type);
}

//! Attempts to save an image to a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILboolean ILAPIENTRY ilSave(ILenum type, ILconst_string FileName)
{
	if (FileName == NULL || iStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILboolean	bRet = IL_FALSE;
	iSetFuncs(&iCurImage->io);
	iCurImage->io.handle = iCurImage->io.openWrite(FileName);
	if (iCurImage->io.handle != NULL) {
		bRet = ilSaveFuncs2(iCurImage, type);
		iCurImage->io.close(iCurImage->io.handle);
		iCurImage->io.handle = NULL;
	}
	return bRet;
}


//! Saves the current image based on the extension given in FileName.
/*! \param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
	       the filename to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILboolean ILAPIENTRY ilSaveImage(ILconst_string FileName)
{
	if (FileName == NULL || iStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILenum type = ilTypeFromExt(FileName);
	ILboolean	bRet = IL_FALSE;
	iSetFuncs(&iCurImage->io);
	iCurImage->io.handle = iCurImage->io.openWrite(FileName);
	if (iCurImage->io.handle != NULL) {
		bRet = ilSaveFuncs2(iCurImage, type);
		iCurImage->io.close(iCurImage->io.handle);
		iCurImage->io.handle = NULL;
	}
	return bRet;
}

//! Attempts to save an image to a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param File File stream to save to.
	\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILuint ILAPIENTRY ilSaveF(ILenum type, ILHANDLE File)
{
	iSetOutputFile(iCurImage, File);
	return ilSaveFuncs2(iCurImage, type);
}


//! Attempts to save an image to a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this image file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
	IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
	IL_VTF, IL_WBMP and IL_JASC_PAL.
	\param Lump Memory buffer to save to
	\param Size Size of the memory buffer
	\return The number of bytes written to the lump, or 0 in case of failure*/
ILuint ILAPIENTRY ilSaveL(ILenum Type, void *Lump, ILuint Size)
{
	iSetOutputLump(iCurImage, Lump, Size);
	ILint64 pos1 = iCurImage->io.tell(iCurImage->io.handle);
	ILboolean bRet = ilSaveFuncs2(iCurImage, Type);
	ILint64 pos2 = iCurImage->io.tell(iCurImage->io.handle);

	if (bRet)
		return pos2-pos1;  // Return the number of bytes written.
	else
		return 0;  // Error occurred
}
