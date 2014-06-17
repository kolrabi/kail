//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 01/06/2009
//
// Filename: IL/devil_internal_exports.h
//
// Description: Internal stuff for DevIL (IL, ILU and ILUT)
//
//-----------------------------------------------------------------------------

#ifndef IL_EXPORTS_H
#define IL_EXPORTS_H

///////////////////////////////////////////////////////////////////////////
//
// Include required headers
//

#include <IL/il.h>
#include <string.h>

#ifdef DEBUG
    #include <assert.h>
#else
    #define assert(x)
#endif

#include <wchar.h>

#ifdef IL_THREAD_SAFE_PTHREAD
#include <pthread.h>
#endif

///////////////////////////////////////////////////////////////////////////
//
// Compiler specific definitions
//

#ifndef INLINE
    #if defined(__GNUC__)
        #define INLINE static inline
    #elif defined(_MSC_VER) //@TODO: Get this working in MSVC++.
        #define INLINE __inline
    #else
        #define INLINE inline
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//! Basic Palette struct
typedef struct ILpal {
    ILubyte* Palette; //!< the image palette (if any)
    ILuint   PalSize; //!< size of the palette (in bytes)
    ILenum   PalType; //!< the palette types in il.h (0x0500 range)
} ILpal;

// Struct for storing IO function pointers and data
typedef struct SIO {
    // Function pointers set by ilSetRead, ilSetWrite
    fOpenProc   openReadOnly;
    fOpenProc   openWrite;
    fCloseProc  close;
    fReadProc   read;
    fSeekProc   seek;
    fEofProc        eof;
    fGetcProc   getchar;
    fTellProc   tell;
    fPutcProc   putchar;
    fWriteProc  write;

    ILint64         lumpPos;
    ILHANDLE        handle;

    const void *lump;
    ILuint          lumpSize, ReadFileStart, WriteFileStart;
} SIO;

#define SIOopenWR(io,       f) ((io)->openWrite    ? (io)->openWrite   ((io)->handle) : NULL)
#define SIOclose( io         ) { if ((io)->close) (io)->close((io)->handle); (io)->handle = NULL; }
#define SIOread(  io, p, s, n) (io)->read   ((io)->handle, (p), (s), (n))
#define SIOseek(  io,    s, w) (io)->seek   ((io)->handle,      (s), (w))
#define SIOgetc(  io         ) (io)->getchar((io)->handle               )
#define SIOtell(  io         ) (io)->tell   ((io)->handle               )
#define SIOputc(  io,       c) (io)->putchar((c), (io)->handle          )
#define SIOwrite( io, p, s, n) (io)->write  ((p), (s), (n), (io)->handle)
#define SIOputs(  io,       s) SIOwrite(io, s, strlen(s), 1)
#define SIOpad(   io,       n) for (ILuint i=0; i<n; i++) SIOputc((io), 0);

/**
 * Open a file using the set functions.
 */
INLINE ILHANDLE SIOopenRO(SIO *io, ILconst_string FileName) {
  if (!io || !FileName)   return NULL;
  if (!io->openReadOnly)  return NULL;

  io->handle = io->openReadOnly(FileName);
  if (io->handle && io->tell && io->seek) {
    ILuint OrigPos = SIOtell(io);
    SIOseek(io, 0, SEEK_END);
    io->lumpSize = SIOtell(io);
    SIOseek(io, OrigPos, SEEK_SET);
  } else {
    io->lumpSize = ~0;
  }
  return io->handle;
}

/**
 * Check whether the file pointer is beyond the end of file.
 */
INLINE ILboolean SIOeof(SIO *io) {
  if (!io) return IL_TRUE;

  if (io->lumpSize != (ILuint)~0 && io->tell) {
    return SIOtell(io) >= io->lumpSize;
  }

  if (io->eof) {
    return io->eof(io->handle);
  }

  return IL_TRUE;
}

ILAPI char *    ILAPIENTRY SIOgets(SIO *io, char *buffer, ILuint maxlen);
ILAPI char *    ILAPIENTRY SIOgetw(SIO *io, char *buffer, ILuint MaxLen);

//! The Fundamental Image structure
/*! Every bit of information about an image is stored in this internal structure.
 * @internal
*/
typedef struct ILimage
{
    ILuint          Width;       //!< the image's width
    ILuint          Height;      //!< the image's height
    ILuint          Depth;       //!< the image's depth
    ILubyte         Bpp;         //!< bytes per pixel (now number of channels)
    ILubyte         Bpc;         //!< bytes per channel
    ILuint          Bps;         //!< bytes per scanline (components for IL)
    ILubyte*        Data;        //!< the image data
    ILuint          SizeOfData;  //!< the total size of the data (in bytes)
    ILuint          SizeOfPlane; //!< SizeOfData in a 2d image, size of each plane slice in a 3d image (in bytes)
    ILenum          Format;      //!< image format (in IL enum style)
    ILenum          Type;        //!< image type (in IL enum style)
    ILenum          Origin;      //!< origin of the image
    ILpal           Pal;         //!< palette details
    ILuint          Duration;    //!< length of the time to display this "frame"
    ILenum          CubeFlags;   //!< cube map flags for sides present in chain
    struct ILimage* Mipmaps;     //!< mipmapped versions of this image terminated by a NULL - usu. NULL
    struct ILimage* Next;        //!< next image in the chain - usu. NULL
    struct ILimage* Faces;       //!< next cubemap face in the chain - usu. NULL
    struct ILimage* Layers;      //!< subsequent layers in the chain - usu. NULL
    void*           Profile;     //!< colour profile
    ILuint          ProfileSize; //!< colour profile size
    ILuint          OffX;        //!< x-offset of the image
    ILuint                  OffY;        //!< y-offset of the image
    ILubyte*        DxtcData;    //!< compressed data
    ILenum          DxtcFormat;  //!< compressed data format
    ILuint          DxtcSize;    //!< compressed data size
    SIO                     io;

#ifdef IL_THREAD_SAFE_PTHREAD
    pthread_mutex_t Mutex;
#endif
} ILimage;

// Memory functions
ILAPI void*         ILAPIENTRY ialloc   (ILsizei Size);
ILAPI void          ILAPIENTRY ifree        (void *Ptr);
ILAPI void*         ILAPIENTRY icalloc  (const ILsizei Size, const ILsizei Num);
#ifdef ALTIVEC_GCC
ILAPI void*         ILAPIENTRY ivec_align_buffer(void *buffer, const ILuint size);
#endif

/** Allocate an array of type @a T with @a n elements. */
#define                    iaalloc(T, n) (T*)icalloc(sizeof(T), (n))

/** Allocate one object of type @a T. */
#define                    ioalloc(T)    iaalloc(T, 1)

// Internal library functions in IL
ILAPI void      ILAPIENTRY iSetError(ILenum Error);
ILAPI void      ILAPIENTRY iSetPal(ILimage *Image, ILpal *Pal);

ILAPI ILimage * ILAPIENTRY iLockCurImage(void);
ILAPI ILimage * ILAPIENTRY iLockImage(ILuint Name);

#ifdef IL_THREAD_SAFE
ILAPI void      ILAPIENTRY iLockState(void);
ILAPI void      ILAPIENTRY iUnlockState(void); 
ILAPI void      ILAPIENTRY iUnlockImage(ILimage *Image);
#else
#define                    iLockState() 
#define                    iUnlockState() 
#define                    iUnlockImage(x)
#endif

//
// Utility functions
//
ILAPI ILubyte ILAPIENTRY ilGetBppFormat(ILenum Format);
ILAPI ILenum  ILAPIENTRY ilGetFormatBpp(ILubyte Bpp);
ILAPI ILubyte ILAPIENTRY ilGetBpcType(ILenum Type);
ILAPI ILenum  ILAPIENTRY ilGetTypeBpc(ILubyte Bpc);
ILAPI ILubyte ILAPIENTRY ilGetBppPal(ILenum PalType);
ILAPI ILenum  ILAPIENTRY ilGetPalBaseType(ILenum PalType);
ILAPI ILuint  ILAPIENTRY ilNextPower2(ILuint Num);
ILAPI ILenum  ILAPIENTRY ilTypeFromExt(ILconst_string FileName);
ILAPI void    ILAPIENTRY iMemSwap(ILubyte *, ILubyte *, const ILuint);

ILAPI wchar_t * ILAPIENTRY iWideFromMultiByte(const char *Multi);
ILAPI char *    ILAPIENTRY iMultiByteFromWide(const wchar_t *Wide);

ILAPI ILboolean ILAPIENTRY iLoad(ILimage *Image, ILenum Type, ILconst_string FileName);
ILAPI void      ILAPIENTRY iSetInputLump(ILimage *, const void *Lump, ILuint Size);
ILAPI void      ILAPIENTRY iSetInputFile(ILimage *, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY iLoadFuncs2(ILimage *Image, ILenum Type);

ILAPI ILboolean ILAPIENTRY iSave(ILimage *Image, ILenum type, ILconst_string FileName);
ILAPI void      ILAPIENTRY iSetOutputLump(ILimage *, void *Lump, ILuint Size);
ILAPI ILboolean ILAPIENTRY iSaveFuncs2(ILimage* image, ILenum type);
ILAPI void      ILAPIENTRY iSetOutputFile(ILimage *, ILHANDLE File);

//
// Image functions
//

ILAPI ILboolean ILAPIENTRY iMirrorImage     (ILimage *Image);
ILAPI ILboolean ILAPIENTRY iFlipImage       (ILimage *Image);
ILAPI ILboolean ILAPIENTRY iClearImage      (ILimage *Image);
ILAPI ILboolean ILAPIENTRY iCopyImage       (ILimage *DestImage, ILimage *SrcImage);;
ILAPI ILuint    ILAPIENTRY iCopyPixels      (ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilCopyImageAttr  (ILimage *Dest, ILimage *Src);
ILAPI ILboolean ILAPIENTRY iConvertImages   (ILimage *BaseImage, ILenum DestFormat, ILenum DestType);
ILAPI ILboolean ILAPIENTRY ilInitImage      (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilIsValidPal     (ILpal *Palette);
ILAPI ILboolean ILAPIENTRY ilResizeImage    (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILboolean ILAPIENTRY iTexImage        (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilTexSubImage_   (ILimage *Image, void *Data);
ILAPI ILimage * ILAPIENTRY iGetMipmap       (ILimage *Image, ILuint Number);
ILAPI ILimage * ILAPIENTRY iGetSubImage     (ILimage *Image, ILuint Number);
ILAPI ILimage*  ILAPIENTRY iConvertImage    (ILimage *Image, ILenum DestFormat, ILenum DestType);
ILAPI ILimage*  ILAPIENTRY ilCopyImage_     (ILimage *Src); // TODO: rename to iCloneImage
ILAPI ILimage*  ILAPIENTRY ilNewImage       (ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILimage*  ILAPIENTRY ilNewImageFull   (ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILint     ILAPIENTRY iGetIntegerImage (ILimage *Image, ILenum Mode);
ILAPI ILpal*    ILAPIENTRY iConvertPal      (ILpal *Pal, ILenum DestFormat);
ILAPI ILboolean ILAPIENTRY iConvertImagePal (ILimage *Image, ILenum DestFormat);
ILAPI ILpal*    ILAPIENTRY iCopyPal        (ILimage *Image); // TODO: rename to iCopyPalFromImage
ILAPI ILubyte*  ILAPIENTRY iGetFlipped     (ILimage *Image);
ILAPI ILuint    ILAPIENTRY ilGetCurName    (void);
ILAPI void      ILAPIENTRY iFlipBuffer(ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num);
ILAPI void      ILAPIENTRY ilCloseImage    (ILimage *Image);
ILAPI void      ILAPIENTRY ilClosePal      (ILpal *Palette);
ILAPI void      ILAPIENTRY ilGetClear      (void *Colours, ILenum Format, ILenum Type);
ILAPI void*     ILAPIENTRY ilConvertBuffer (ILuint SizeOfData, ILenum SrcFormat, ILenum DestFormat, ILenum SrcType, ILenum DestType, ILpal *SrcPal, void *Buffer);

ILAPI ILuint    ILAPIENTRY iGetDXTCData(ILimage *Image, void *Buffer, ILuint BufferSize, ILenum DXTCFormat);


#ifdef _UNICODE
  #ifndef _WIN32  // At least in Linux, fopen works fine, and wcsicmp is not defined.
    #define iStrICmp wcscasecmp
    #define _wfopen fopen
  #else
    #define iStrIcmp wcsicmp
  #endif
  #define iStrCmp wcscmp
  #define iStrCpy wcscpy
  #define iStrCat wcscat
  #define iStrLen wcslen
#else
  #define iStrIcmp iCharStrICmp

  #define iStrCmp strcmp
  #define iStrCpy strcpy
  #define iStrCat strcat
  #define iStrLen strlen
#endif

#define iCharStrLen strlen
#define iCharStrCpy strcpy
#ifdef _WIN32
  #define iCharStrICmp stricmp
#else
  #define iCharStrICmp strcasecmp
#endif

ILAPI ILstring  ILAPIENTRY iStrDup(ILconst_string Str);
ILAPI char *    ILAPIENTRY iCharStrDup(const char *Str);
ILAPI ILstring  ILAPIENTRY iGetExtension(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY iCheckExtension(ILconst_string Arg, ILconst_string Ext);

//
// ILU functions
//
ILAPI ILboolean ILAPIENTRY iSwapColours(ILimage *img);
ILAPI ILboolean ILAPIENTRY iFlipImage(ILimage *image);
ILAPI ILimage*  ILAPIENTRY iluRotate_(ILimage *Image, ILfloat Angle);
ILAPI ILimage*  ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle);
ILAPI ILimage*  ILAPIENTRY iluScale_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean ILAPIENTRY iBuildMipmaps(ILimage *Parent, ILuint Width, ILuint Height, ILuint Depth);

#ifdef __cplusplus
}
#endif

#endif//IL_EXPORTS_H
