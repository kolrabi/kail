//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/01/2009
//
// Filename: src-IL/include/il_internal.h
//
// Description: Internal stuff for DevIL
//
//-----------------------------------------------------------------------------
#ifndef INTERNAL_H
#define INTERNAL_H
#define _IL_BUILD_LIBRARY

// Local headers
#include <IL/config.h>

// Standard headers
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <IL/il.h>
#include <IL/devil_internal_exports.h>
#include "il_files.h"
#include "il_endian.h"
#include "il_string.h"
#include "il_pal.h"

// Windows-specific
#ifdef _WIN32
	#ifdef _MSC_VER
		#if _MSC_VER > 1000
			#pragma once
			#pragma intrinsic(memcpy)
			#pragma intrinsic(memset)
			#pragma intrinsic(strcmp)
			#pragma intrinsic(strlen)
			#pragma intrinsic(strcpy)
			
			#if _MSC_VER >= 1300
				#pragma warning(disable : 4996)  // MSVC++ 8/9 deprecation warnings
			#endif
		#endif // _MSC_VER > 1000
	#endif
	#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
	#include <windows.h>
#endif//_WIN32


#ifdef IL_INLINE_ASM
	#if (defined (_MSC_VER) && defined(_WIN32))  // MSVC++ only
		#define USE_WIN32_ASM
	#endif

	#ifdef _WIN64
		#undef USE_WIN32_ASM
	//@TODO: Windows 64 compiler cannot use inline ASM, so we need to
	//  generate some MASM code at some point.
	#endif

	#ifdef _WIN32_WCE  // Cannot use our inline ASM in Windows Mobile.
		#undef USE_WIN32_ASM
	#endif
#endif

#define BIT(n) (1<<n)
#define BIT_0	0x00000001
#define BIT_1	0x00000002
#define BIT_2	0x00000004
#define BIT_3	0x00000008
#define BIT_4	0x00000010
#define BIT_5	0x00000020
#define BIT_6	0x00000040
#define BIT_7	0x00000080
#define BIT_8	0x00000100
#define BIT_9	0x00000200
#define BIT_10	0x00000400
#define BIT_11	0x00000800
#define BIT_12	0x00001000
#define BIT_13	0x00002000
#define BIT_14	0x00004000
#define BIT_15	0x00008000
#define BIT_16	0x00010000
#define BIT_17	0x00020000
#define BIT_18	0x00040000
#define BIT_19	0x00080000
#define BIT_20	0x00100000
#define BIT_21	0x00200000
#define BIT_22	0x00400000
#define BIT_23	0x00800000
#define BIT_24	0x01000000
#define BIT_25	0x02000000
#define BIT_26	0x04000000
#define BIT_27	0x08000000
#define BIT_28	0x10000000
#define BIT_29	0x20000000
#define BIT_30	0x40000000
#define BIT_31	0x80000000
#define NUL '\0'  // Easier to type and ?portable?

//
// Some math functions
//

// A fast integer squareroot, completely accurate for x < 289.
// Taken from http://atoms.org.uk/sqrt/
// There is also a version that is accurate for all integers
// < 2^31, if we should need it
int iSqrt(int x);

//
// Useful miscellaneous functions
//

ILboolean			iFileExists(ILconst_string FileName);
void					ilDefaultStates(void);
ILenum				iGetHint(ILenum Target);
ILint					iGetInt(ILenum Mode);
void					ilRemoveRegistered(void);
ILenum 				iGetError(void);
ILconst_string iGetILString(ILenum StringName);
void 					iHint(ILenum Target, ILenum Mode);
//
// Rle compression
//

#define		IL_TGACOMP 0x01
#define		IL_PCXCOMP 0x02
#define		IL_SGICOMP 0x03
#define   IL_BMPCOMP 0x04

ILboolean	ilRleCompressLine(ILubyte *ScanLine, ILuint Width, ILubyte Bpp, ILubyte *Dest, ILuint *DestWidth, ILenum CompressMode);
ILuint		ilRleCompress(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte *Dest, ILenum CompressMode, ILuint *ScanTable);

// DXTC compression

ILuint						ilNVidiaCompressDXTFile(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtType);
ILubyte*	iNVidiaCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);
ILubyte*	iSquishCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);
ILubyte*    iCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DXTCFormat, ILuint *DXTCSize);
ILboolean iDxtcDataToImage(ILimage* image);
ILboolean iDxtcDataToSurface(ILimage* image);
ILboolean iSurfaceToDxtcData(ILimage* image, ILenum Format);
void iFlipSurfaceDxtcData(ILimage* image);

// Conversion functions
// ILboolean	ilAddAlpha(void);
ILboolean	iAddAlpha(ILimage *Image);
ILboolean	iRemoveAlpha(ILimage *Image);
ILboolean	iAddAlphaKey(ILimage *Image);
ILboolean	iFastConvert(ILimage *Image, ILenum DestFormat);
ILboolean	iSwapColours(ILimage *Image);
ILboolean iFixImages(ILimage *Image);
ILboolean iApplyProfile(ILimage *Image, ILstring InProfile, ILstring OutProfile);

// Miscellaneous functions
char*		iGetString(ILenum StringName);  // Internal version of ilGetString
ILuint iDuplicateImage(ILuint SrcName);
ILboolean iBlit(ILimage *Image, ILimage *Src, ILint DestX,  ILint DestY,   ILint DestZ, 
                                           ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
                                           ILuint Width, ILuint Height, ILuint Depth);
void iClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
ILboolean iOverlayImage(ILimage *Dest, ILimage *Src, ILint XCoord, ILint YCoord, ILint ZCoord);
ILubyte* iGetData(ILimage *Image);
ILuint iGetDXTCData(ILimage *Image, void *Buffer, ILuint BufferSize, ILenum DXTCFormat);
ILubyte *iGetPalette(ILimage *Image);
ILboolean iInvertSurfaceDxtcDataAlpha(ILimage* image);
ILboolean iImageToDxtcData(ILimage *image, ILenum Format);
ILboolean iSetData(ILimage *Image, void *Data);
ILboolean iSetDuration(ILimage *Image, ILuint Duration);
ILboolean iTexImageDxtc(ILimage* image, ILint w, ILint h, ILint d, ILenum DxtFormat, const ILubyte* data);

// TODO: put all functions that have a corresponding il* public api function equivalent into internal exports

//
// Image loading/saving functions
//
#include "il_formats.h"

void iSetOutputFake(ILimage *image);
void iSetOutputLump(ILimage *image, void *Lump, ILuint Size);
ILuint iDetermineSize(ILimage *Image, ILenum Type);
ILenum iDetermineType(ILimage *Image, ILconst_string FileName);
ILenum iDetermineTypeFuncs(ILimage *Image);
ILuint64 iGetLumpPos(ILimage *Image) ;
ILboolean iIsValidIO(ILenum Type, SIO* io);
void iSetInputLumpIO(SIO *io, const void *Lump, ILuint Size);
ILboolean iLoad(ILimage *Image, ILenum Type, ILconst_string FileName);
ILboolean iLoadFuncs2(ILimage* image, ILenum type);
ILboolean iSave(ILimage *Image, ILenum type, ILconst_string FileName);
ILboolean iSaveFuncs2(ILimage* image, ILenum type);
ILboolean iSaveImage(ILimage *Image, ILconst_string FileName);
void iSetRead(ILimage *Image, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, 
  fReadProc aRead, fSeekProc aSeek, fTellProc aTell);
void iResetRead(ILimage *image);
void iSetWrite(ILimage *Image, fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek, 
  fTellProc Tell, fWriteProc Write);
void iResetWrite(ILimage *image);

ILenum iTypeFromExt(ILconst_string FileName);

ILboolean iLoadData(ILimage *Image, ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILboolean iLoadDataF(ILimage *Image, ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILboolean iLoadDataInternal(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILboolean iSaveData(ILimage *Image, ILconst_string FileName);

 
/* FIXME:
ILboolean ilIsValidExr(ILconst_string FileName);
ILboolean ilIsValidExrF(ILHANDLE File);
ILboolean ilIsValidExrL(const void *Lump, ILuint Size);

ILboolean ilLoadExr(ILconst_string FileName);
ILboolean ilLoadExrF(ILHANDLE File);
ILboolean ilLoadExrL(const void *Lump, ILuint Size);
ILboolean iLoadExrInternal();
*/

/* FIXME:
ILboolean ilIsValidWdp(ILconst_string FileName);
ILboolean ilIsValidWdpF(ILHANDLE File);
ILboolean ilIsValidWdpL(const void *Lump, ILuint Size);
ILboolean ilLoadWdp(ILconst_string FileName);
ILboolean ilLoadWdpF(ILHANDLE File);
ILboolean ilLoadWdpL(const void *Lump, ILuint Size);
*/

// OpenEXR is written in C++, so we have to wrap this to avoid linker errors.
/*#ifndef IL_NO_EXR
	#ifdef __cplusplus
	extern "C" {
	#endif
		ILboolean ilLoadExr(ILconst_string FileName);
	#ifdef __cplusplus
	}
	#endif
#endif*/

//ILboolean ilLoadExr(ILconst_string FileName);

#define imemclear(x,y) memset(x,0,y);

extern FILE *iTraceOut;

#define iTrace(...) if (iTraceOut) {\
	fprintf(iTraceOut, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(iTraceOut, __VA_ARGS__); \
	fputc('\n', iTraceOut); \
	fflush(iTraceOut); \
}

#define iTraceV(fmt, args) if (iTraceOut) {\
	fprintf(iTraceOut, "%s:%d: ", __FILE__, __LINE__); \
	vfprintf(iTraceOut, fmt, args); \
	fputc('\n', iTraceOut); \
	fflush(iTraceOut); \
}

#ifdef DEBUG
#define iAssert(x) { \
	bool __x = (x); \
	iTrace("Assertion failed: ", #x); \
	assert(x); \
}
#else
#define iAssert(x)
#endif

#ifdef __cplusplus
}
#endif

#endif//INTERNAL_H
