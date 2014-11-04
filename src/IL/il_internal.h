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
#include "il_states.h"

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

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ ""
#endif

#define IL_ATTRIB_STACK_MAX         32
#define IL_ERROR_STACK_SIZE         32

typedef struct {
  ILuint    CurName;
  ILuint    CurFrame;
  ILuint    CurFace;
  ILuint    CurLayer;
  ILuint    CurMipmap;
} IL_IMAGE_SELECTION;

typedef struct {
  ILuint    ilCurrentPos;  // Which position on the stack
  IL_STATES ilStates[IL_ATTRIB_STACK_MAX];
  IL_HINTS  ilHints;
} IL_STATE_STRUCT;

typedef struct {
  ILenum  ilErrorNum[IL_ERROR_STACK_SIZE];
  ILint   ilErrorPlace;
} IL_ERROR_STACK;

typedef struct {
  IL_IMAGE_SELECTION CurSel;
  IL_STATE_STRUCT    CurState;
  IL_ERROR_STACK     CurError;
} IL_TLS_DATA;

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

ILboolean     iFileExists(ILconst_string FileName);
void          ilDefaultStates(void);
ILenum        iGetHint(ILenum Target);
ILint         iGetInt(ILenum Mode);

void          ilRemoveRegistered(void);
ILenum        iGetError(void);
ILconst_string iGetILString(ILimage *Image, ILenum StringName);
void          iHint(ILenum Target, ILenum Mode);

IL_TLS_DATA * iGetTLSData(void);

// In il_neuquant.c
ILimage *iNeuQuant(ILimage *Image, ILuint NumCols);
// In il_quantizer.c
ILimage *iQuantizeImage(ILimage *Image, ILuint NumCols);
ILimage *iConvertPalette(ILimage *Image, ILenum DestFormat);

//
// DXTC compression
//

ILubyte*  iSquishCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);
ILubyte*  iCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DXTCFormat, ILuint *DXTCSize);
ILboolean iDxtcDataToImage(ILimage* image);
ILboolean iDxtcDataToSurface(ILimage* image);
ILboolean iSurfaceToDxtcData(ILimage* image, ILenum Format);
void      iFlipSurfaceDxtcData(ILimage* image);

// Conversion functions
ILboolean iAddAlpha(ILimage *Image);
ILboolean iRemoveAlpha(ILimage *Image);
ILboolean iAddAlphaKey(ILimage *Image);
ILboolean iFastConvert(ILimage *Image, ILenum DestFormat);
ILboolean iFixImages(ILimage *Image, ILimage *BaseImage);
ILboolean iApplyProfile(ILimage *Image, ILstring InProfile, ILstring OutProfile);

// Miscellaneous functions
char*     iGetString(ILimage *Image, ILenum StringName);  // Internal version of ilGetString
ILuint    iDuplicateImage(ILimage *Image);
ILboolean iBlit(ILimage *Image, ILimage *Src, ILint DestX,  ILint DestY,   ILint DestZ, 
                                           ILuint SrcX,  ILuint SrcY,   ILuint SrcZ,
                                           ILuint Width, ILuint Height, ILuint Depth);
void      iClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
void      iClearIndex(ILuint Index);
ILboolean iOverlayImage(ILimage *Dest, ILimage *Src, ILint XCoord, ILint YCoord, ILint ZCoord);
ILubyte * iGetData(ILimage *Image);
ILubyte * iGetPalette(ILimage *Image);
ILboolean iInvertSurfaceDxtcDataAlpha(ILimage* image);
ILboolean iImageToDxtcData(ILimage *image, ILenum Format);
ILboolean iSetData(ILimage *Image, void *Data);
ILboolean iSetDuration(ILimage *Image, ILuint Duration);
ILboolean iTexImageDxtc(ILimage* image, ILuint w, ILuint h, ILuint d, ILenum DxtFormat, const ILubyte* data);

// TODO: put all functions that have a corresponding il* public api function equivalent into internal exports

//
// Image loading/saving functions
//
#include "il_formats.h"

void      iSetOutputFake(ILimage *image);
ILuint    iDetermineSize(ILimage *Image, ILenum Type);
ILenum    iDetermineType(ILimage *Image, ILconst_string FileName);
ILenum    iDetermineTypeFuncs(ILimage *Image);
ILuint64  iGetLumpPos(ILimage *Image) ;
ILboolean iIsValidIO(ILenum Type, SIO* io);
void      iSetInputLumpIO(SIO *io, void *Lump, ILuint Size);
void      iSetRead(ILimage *Image, fOpenProc aOpen, fCloseProc aClose, fEofProc aEof, fGetcProc aGetc, fReadProc aRead, fSeekProc  aSeek, fTellProc aTell);
void      iSetWrite(ILimage *Image, fOpenProc Open, fCloseProc Close, fPutcProc Putc, fSeekProc Seek,  fTellProc Tell,  fWriteProc Write);
void      iResetRead(ILimage *image);
void      iResetWrite(ILimage *image);

ILboolean iLoadData(ILimage *Image, ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILboolean iLoadDataF(ILimage *Image, ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILboolean iLoadDataInternal(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILboolean iSaveData(ILimage *Image, ILconst_string FileName);

 /* FIXME:
ILboolean ilIsValidWdp(ILconst_string FileName);
ILboolean ilIsValidWdpF(ILHANDLE File);
ILboolean ilIsValidWdpL(const void *Lump, ILuint Size);
ILboolean ilLoadWdp(ILconst_string FileName);
ILboolean ilLoadWdpF(ILHANDLE File);
ILboolean ilLoadWdpL(const void *Lump, ILuint Size);
*/

ILboolean iIsValidDds(SIO* io);
ILboolean iLoadDdsInternal(ILimage* image);

ILboolean iExifLoad(ILimage *Image);
ILboolean iExifSave(ILimage *Image);

ILboolean iEnumMetadata(ILimage *Image, ILuint Index, ILenum *IFD, ILenum *ID);
ILboolean iGetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum *Type, ILuint *Count, ILuint *Size, void **Data);
ILboolean iSetMetadata(ILimage *Image, ILenum IFD, ILenum ID, ILenum Type, ILuint Count, ILuint Size, const void *Data);
void      iClearMetadata(ILimage *Image);

#ifdef __cplusplus
}
#endif

#endif//INTERNAL_H
