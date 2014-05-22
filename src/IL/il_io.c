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

	if (FileName == NULL || ilStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_TYPE_UNKNOWN;
	}

	Ext = iGetExtension(FileName);
	//added 2003-08-31: fix sf bug 789535
	if (Ext == NULL) {
		return IL_TYPE_UNKNOWN;
	}

	if (!iStrCmp(Ext, IL_TEXT("tga")) || !iStrCmp(Ext, IL_TEXT("vda")) ||
		!iStrCmp(Ext, IL_TEXT("icb")) || !iStrCmp(Ext, IL_TEXT("vst")))
		Type = IL_TGA;
	else if (!iStrCmp(Ext, IL_TEXT("jpg")) || !iStrCmp(Ext, IL_TEXT("jpe")) ||
		!iStrCmp(Ext, IL_TEXT("jpeg")) || !iStrCmp(Ext, IL_TEXT("jif")) || !iStrCmp(Ext, IL_TEXT("jfif")))
		Type = IL_JPG;
	else if (!iStrCmp(Ext, IL_TEXT("jp2")) || !iStrCmp(Ext, IL_TEXT("jpx")) ||
		!iStrCmp(Ext, IL_TEXT("j2k")) || !iStrCmp(Ext, IL_TEXT("j2c")))
		Type = IL_JP2;
	else if (!iStrCmp(Ext, IL_TEXT("png")))
		Type = IL_PNG;
	else if (!iStrCmp(Ext, IL_TEXT("exr")))
		Type = IL_EXR;
	else if (!iStrCmp(Ext, IL_TEXT("iff")))
		Type = IL_IFF;
	else if (!iStrCmp(Ext, IL_TEXT("ilbm")) || !iStrCmp(Ext, IL_TEXT("lbm")) ||
        !iStrCmp(Ext, IL_TEXT("ham")))
		Type = IL_ILBM;
	else if (!iStrCmp(Ext, IL_TEXT("iwi")))
		Type = IL_IWI;
	else if (!iStrCmp(Ext, IL_TEXT("iwi")))
		Type = IL_IWI;
	else if (!iStrCmp(Ext, IL_TEXT("jng")))
		Type = IL_JNG;
	else if (!iStrCmp(Ext, IL_TEXT("lif")))
		Type = IL_LIF;
	else if (!iStrCmp(Ext, IL_TEXT("mdl")))
		Type = IL_MDL;
	else if (!iStrCmp(Ext, IL_TEXT("mng")) || !iStrCmp(Ext, IL_TEXT("jng")))
		Type = IL_MNG;
	else if (!iStrCmp(Ext, IL_TEXT("mp3")))
		Type = IL_MP3;
	else if (!iStrCmp(Ext, IL_TEXT("pcd")))
		Type = IL_PCD;
	else if (!iStrCmp(Ext, IL_TEXT("pcx")))
		Type = IL_PCX;
	else if (!iStrCmp(Ext, IL_TEXT("pic")))
		Type = IL_PIC;
	else if (!iStrCmp(Ext, IL_TEXT("pix")))
		Type = IL_PIX;
	else if (!iStrCmp(Ext, IL_TEXT("pbm")) || !iStrCmp(Ext, IL_TEXT("pgm")) ||
		!iStrCmp(Ext, IL_TEXT("pnm")) || !iStrCmp(Ext, IL_TEXT("ppm")))
		Type = IL_PNM;
	else if (!iStrCmp(Ext, IL_TEXT("psd")) || !iStrCmp(Ext, IL_TEXT("pdd")))
		Type = IL_PSD;
	else if (!iStrCmp(Ext, IL_TEXT("psp")))
		Type = IL_PSP;
	else if (!iStrCmp(Ext, IL_TEXT("pxr")))
		Type = IL_PXR;
	else if (!iStrCmp(Ext, IL_TEXT("rot")))
		Type = IL_ROT;
	else if (!iStrCmp(Ext, IL_TEXT("sgi")) || !iStrCmp(Ext, IL_TEXT("bw")) ||
		!iStrCmp(Ext, IL_TEXT("rgb")) || !iStrCmp(Ext, IL_TEXT("rgba")))
		Type = IL_SGI;
	else if (!iStrCmp(Ext, IL_TEXT("sun")) || !iStrCmp(Ext, IL_TEXT("ras")) ||
			 !iStrCmp(Ext, IL_TEXT("rs")) || !iStrCmp(Ext, IL_TEXT("im1")) ||
			 !iStrCmp(Ext, IL_TEXT("im8")) || !iStrCmp(Ext, IL_TEXT("im24")) ||
			 !iStrCmp(Ext, IL_TEXT("im32")))
		Type = IL_SUN;
	else if (!iStrCmp(Ext, IL_TEXT("texture")))
		Type = IL_TEXTURE;
	else if (!iStrCmp(Ext, IL_TEXT("tpl")))
		Type = IL_TPL;
	else if (!iStrCmp(Ext, IL_TEXT("utx")))
		Type = IL_UTX;
	else if (!iStrCmp(Ext, IL_TEXT("vtf")))
		Type = IL_VTF;
	else if (!iStrCmp(Ext, IL_TEXT("wal")))
		Type = IL_WAL;
	else if (!iStrCmp(Ext, IL_TEXT("wbmp")))
		Type = IL_WBMP;
	else if (!iStrCmp(Ext, IL_TEXT("wdp")) || !iStrCmp(Ext, IL_TEXT("hdp")))
		Type = IL_WDP;
	else if (!iStrCmp(Ext, IL_TEXT("xpm")))
		Type = IL_XPM;
	else
		iIdentifyFormatExt(Ext);

	return Type;
}


//changed 2003-09-17 to ILAPIENTRY
ILenum ILAPIENTRY ilDetermineType(ILconst_string FileName)
{
	ILenum Type = IL_TYPE_UNKNOWN;

	if (FileName != NULL) {
		// If we can open the file, determine file type from contents
		// This is more reliable than the file name extension 
		ilResetRead();
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
		case 0x01:
			if (buf[1] == 0xda)
				#ifndef IL_NO_SGI
				if (iIsValidSgi())
				#endif
					return IL_SGI;
			break;
		#ifndef IL_NO_PCX
		case 0x0a:
			if (iIsValidPcx(&iCurImage->io))
				return IL_PCX;
			break;
		#endif
		case '8':
			if (buf[1] == 'B' && buf[2] == 'P' && buf[3] == 'S')
				#ifndef IL_NO_PSD
				if (iIsValidPsd())
					return IL_PSD;
				#else
				return IL_PSD;
				#endif
			break;
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
		case 'P':
			if (strnicmp((const char*) buf, "Paint Shop Pro Image File", 25) == 0) {
				#ifndef IL_NO_PSP
				if (iIsValidPsp())
				#endif
					return IL_PSP;
			} else if (buf[1] >= '1' && buf[1] <= '6') {
					return IL_PNM; // il_pnm's test doesn't add anything here
			}
			break;
		case 'V':
			if (buf[1] == 'T' && buf[2] == 'F')
				#ifndef IL_NO_VTF
				if (iIsValidVtf(&iCurImage->io))
					return IL_VTF;
				#else
				return IL_VTF;
				#endif
			break;
		case 0x59:
			if (buf[1] == 0xA6 && buf[2] == 0x6A && buf[3] == 0x95)
				#ifndef IL_NO_SUN
				if (iIsValidSun(&iCurImage->io))
					return IL_SUN;
				#endif
			break;
		case 'v':
			if (buf[1] == '/' && buf[2] == '1' && buf[3] == 1)
				#ifndef IL_NO_EXR
				if (iIsValidExr())
				#endif
					return IL_EXR;
			break;
		case 0x89:
			if (buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G')
				#ifndef IL_NO_PNG
				if (iIsValidPng(&iCurImage->io))
					return IL_PNG;
				#else
				return IL_PNG;
				#endif
			break;
		case 0x8a:
			if (buf[1] == 0x4D
			&&  buf[2] == 0x4E
			&&  buf[3] == 0x47
			&&  buf[4] == 0x0D
			&&  buf[5] == 0x0A
			&&  buf[6] == 0x1A
			&&  buf[7] == 0x0A)
			{
				return IL_MNG;
			}
			break;
		case 0xff:
			if (buf[1] == 0xd8)
				return IL_JPG;
			break;
	}

	#ifndef IL_NO_ILBM
	if (iIsValidIlbm())
		return IL_ILBM;
	#endif

	#ifndef IL_NO_IWI
	if (ilIsValidIwiF(File))
		return IL_IWI;
	#endif

	#ifndef IL_NO_JP2
	if (iIsValidJp2(&iCurImage->io))
		return IL_JP2;
	#endif

	#ifndef IL_NO_LIF
	if (ilIsValidLifF(File))
		return IL_LIF;
	#endif

	#ifndef IL_NO_MDL
	if (ilIsValidMdlF(File))
		return IL_MDL;
	#endif

	#ifndef IL_NO_MDL
	if (ilIsValidMp3F(File))
		return IL_MP3;
	#endif

	#ifndef IL_NO_PIC
	if (iIsValidPic(&iCurImage->io))
		return IL_PIC;
	#endif

	#ifndef IL_NO_PIX
	if (iIsValidPix(&iCurImage->io))
		return IL_PIX;
	#endif

	#ifndef IL_NO_TPL
	if (ilIsValidTplF(File))
		return IL_TPL;
	#endif

	#ifndef IL_NO_XPM
	if (iIsValidXpm(&iCurImage->io))
		return IL_XPM;
	#endif

	return iIdentifyFormat(&iCurImage->io);
}

ILenum ILAPIENTRY ilDetermineTypeF(ILHANDLE File)
{
	iSetInputFile(File);
	return ilDetermineTypeFuncs();
}

ILenum ILAPIENTRY ilDetermineTypeL(const void *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
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
		#ifndef IL_NO_JPG
		case IL_JPG:
			return iIsValidJpeg(io);
		#endif

		#ifndef IL_NO_PNG
		case IL_PNG:
			return iIsValidPng(io);
		#endif

		#ifndef IL_NO_EXR
		case IL_EXR:
			return ilIsValidExr(io);
		#endif

		#ifndef IL_NO_IWI
		case IL_IWI:
			return ilIsValidIwi(io);
		#endif

    	#ifndef IL_NO_ILBM
        case IL_ILBM:
            return iIsValidIlbm();
	    #endif

		#ifndef IL_NO_JP2
		case IL_JP2:
			return iIsValidJp2(io);
		#endif

		#ifndef IL_NO_LIF
		case IL_LIF:
			return ilIsValidLif(io);
		#endif

		#ifndef IL_NO_MDL
		case IL_MDL:
			return ilIsValidMdl(io);
		#endif

		#ifndef IL_NO_MP3
		case IL_MP3:
			return iIsValidMp3(io);
		#endif

		#ifndef IL_NO_PCX
		case IL_PCX:
			return iIsValidPcx(io);
		#endif

		#ifndef IL_NO_PIC
		case IL_PIC:
			return iIsValidPic(io);
		#endif

		#ifndef IL_NO_PNM
		case IL_PNM:
			return iIsValidPnm();
		#endif

		#ifndef IL_NO_PSD
		case IL_PSD:
			return iIsValidPsd();
		#endif

		#ifndef IL_NO_PSP
		case IL_PSP:
			return iIsValidPsp();
		#endif

		#ifndef IL_NO_SGI
		case IL_SGI:
			return iIsValidSgi();
		#endif

		#ifndef IL_NO_SUN
		case IL_SUN:
			return iIsValidSun(io);
		#endif

		#ifndef IL_NO_TPL
		case IL_TPL:
			return ilIsValidTpl(io);
		#endif

		#ifndef IL_NO_VTF
		case IL_VTF:
			return iIsValidVtf(io);
		#endif

		#ifndef IL_NO_XPM
		case IL_XPM:
			return iIsValidXpm(io);
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
		ilResetRead();
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
	iSetInputFile(File);
	return iIsValid(Type, &iCurImage->io);
}


ILboolean ILAPIENTRY ilIsValidL(ILenum Type, void *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
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
	if (FileName == NULL || ilStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	ilResetRead();
	iCurImage->io.handle = iCurImage->io.openReadOnly(FileName);
	if (iCurImage->io.handle != NULL) {
		ILboolean bRet = ilLoadFuncs(Type);
		iCurImage->io.close(iCurImage->io.handle);

		if (Type == IL_CUT) {
			// Attempt to load the palette
			size_t fnLen = ilStrLen(FileName);
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

	ilResetRead();
	iCurImage->io.handle = File;
	iCurImage->io.seek(iCurImage->io.handle, 0, IL_SEEK_SET);
	if (Type == IL_TYPE_UNKNOWN)
		Type = ilDetermineTypeFuncs();

	ILboolean bRet = ilLoadFuncs(Type);

	return bRet;
}

ILboolean ILAPIENTRY ilLoadFuncs2(ILimage* image, ILenum type)
{
	switch (type)
	{
		case IL_TYPE_UNKNOWN:
			return IL_FALSE;

		#ifndef IL_NO_JPG
			#ifndef IL_USE_IJL
			case IL_JPG:
				return iLoadJpegInternal(image);
			#endif
		#endif

		#ifndef IL_NO_ILBM
		case IL_ILBM:
			return iLoadIlbmInternal();
		#endif

		#ifndef IL_NO_PCD
		case IL_PCD:
			return iLoadPcdInternal(image);
		#endif

		#ifndef IL_NO_PCX
		case IL_PCX:
			return iLoadPcxInternal(image);
		#endif

		#ifndef IL_NO_PIC
		case IL_PIC:
			return iLoadPicInternal(image);
		#endif

		#ifndef IL_NO_PNG
		case IL_PNG:
			return iLoadPngInternal(image);
		#endif

		#ifndef IL_NO_PNM
		case IL_PNM:
			return iLoadPnmInternal();
		#endif

		#ifndef IL_NO_SGI
		case IL_SGI:
			return iLoadSgiInternal();
		#endif

		#ifndef IL_NO_RAW
		case IL_RAW:
			return iLoadRawInternal();
		#endif

		// Currently broken - need wrappers for streams?
		/*#ifndef IL_NO_JP2
		case IL_JP2:
			return iLoadJp2Internal(image);
		#endif*/

		#ifndef IL_NO_MNG
		case IL_MNG:
			return iLoadMngInternal();
		#endif

		#ifndef IL_NO_PSD
		case IL_PSD:
			return iLoadPsdInternal(image);
		#endif

		#ifndef IL_NO_PSP
		case IL_PSP:
			return iLoadPspInternal();
		#endif

		#ifndef IL_NO_PIX
		case IL_PIX:
			return iLoadPixInternal(image);
		#endif

		#ifndef IL_NO_PXR
		case IL_PXR:
			return iLoadPxrInternal();
		#endif

		#ifndef IL_NO_XPM
		case IL_XPM:
			return iLoadXpmInternal();
		#endif

		#ifndef IL_NO_VTF
		case IL_VTF:
			return iLoadVtfInternal(iCurImage);
		#endif

		#ifndef IL_NO_WBMP
		case IL_WBMP:
			return iLoadWbmpInternal(&iCurImage->io);
		#endif

		#ifndef IL_NO_SUN
		case IL_SUN:
			return iLoadSunInternal(iCurImage);
		#endif

		#ifndef IL_NO_IFF
		case IL_IFF:
			return iLoadIffInternal();
		#endif

		#ifndef IL_NO_TEXTURE
		case IL_TEXTURE:
			//return ilLoadTextureF(File);
			// From http://forums.totalwar.org/vb/showthread.php?t=70886, all that needs to be done
			//  is to strip out the first 48 bytes, and then it is DDS data.
			image->io.seek(image->io.handle, 48, IL_SEEK_CUR);
			return ilLoadFuncs2(image, IL_DDS);
		#endif

		#ifndef IL_NO_EXR
		case IL_EXR:
			return iLoadExrInternal();
		#endif

		#ifndef IL_NO_IWI
		case IL_IWI:
			return ilLoadIwiF(File);
		#endif

		#ifndef IL_NO_LIF
		case IL_LIF:
			return ilLoadLifF(File);
		#endif

		#ifndef IL_NO_MDL
		case IL_MDL:
			return ilLoadMdlF(File);
		#endif

		#ifndef IL_NO_MP3
		case IL_MP3:
			return iLoadMp3Internal(image);
		#endif

		#ifndef IL_NO_ROT
		case IL_ROT:
			return ilLoadRotF(File);
		#endif

		#ifndef IL_NO_TPL
		case IL_TPL:
			return ilLoadTplF(File);
		#endif

		#ifndef IL_NO_UTX
		case IL_UTX:
			return ilLoadUtxF(File);
		#endif

		#ifndef IL_NO_WAL
		case IL_WAL:
			return ilLoadWalF(File);
		#endif
	}

	iTrace("----");
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
	if (type == IL_TYPE_UNKNOWN)
		type = ilDetermineTypeFuncs();

	return ilLoadFuncs2(iCurImage, type);
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

	iSetInputLump(Lump, Size);
	return ilLoadFuncs(Type);
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

	switch(type) {
	#ifndef IL_NO_JPG
	case IL_JPG:
		bRet = iSaveJpegInternal(image);
		break;
	#endif

	#ifndef IL_NO_PCX
	case IL_PCX:
		bRet = iSavePcxInternal(image);
		break;
	#endif

	#ifndef IL_NO_PNG
	case IL_PNG:
		bRet = iSavePngInternal(image);
		break;
	#endif

	#ifndef IL_NO_PNM  // Not sure if binary or ascii should be defaulted...maybe an option?
	case IL_PNM:
		bRet = iSavePnmInternal();
		break;
	#endif

	#ifndef IL_NO_SGI
	case IL_SGI:
		bRet = iSaveSgiInternal();
		break;
	#endif

	#ifndef IL_NO_RAW
	case IL_RAW:
		bRet = iSaveRawInternal();
		break;
	#endif

	#ifndef IL_NO_MNG
	case IL_MNG:
		bRet = iSaveMngInternal();
		break;
	#endif

	#ifndef IL_NO_PSD
	case IL_PSD:
		bRet = iSavePsdInternal(image);
		break;
	#endif

	#ifndef IL_NO_JP2
	case IL_JP2:
		bRet = iSaveJp2Internal(image);
		break;
	#endif

	#ifndef IL_NO_EXR
	case IL_EXR:
		bRet = iSaveExrInternal(FileName);
		break;
	#endif

	#ifndef IL_NO_VTF
	case IL_VTF:
		bRet = iSaveVtfInternal(iCurImage);
		break;
	#endif

	#ifndef IL_NO_WBMP
	case IL_WBMP:
		bRet = iSaveWbmpInternal(&iCurImage->io);
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
	if (FileName == NULL || ilStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILboolean	bRet = IL_FALSE;
	ilResetWrite();
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
	if (FileName == NULL || ilStrLen(FileName) < 1) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ILenum type = ilTypeFromExt(FileName);
	ILboolean	bRet = IL_FALSE;
	ilResetWrite();
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
	iSetOutputFile(File);
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
	iSetOutputLump(Lump, Size);
	ILint64 pos1 = iCurImage->io.tell(iCurImage->io.handle);
	ILboolean bRet = ilSaveFuncs2(iCurImage, Type);
	ILint64 pos2 = iCurImage->io.tell(iCurImage->io.handle);

	if (bRet)
		return pos2-pos1;  // Return the number of bytes written.
	else
		return 0;  // Error occurred
}
