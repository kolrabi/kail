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

#if defined(IL_THREAD_SAFE_PTHREAD)
#include <pthread.h>
#elif defined(IL_THREAD_SAFE_WIN32)
#define WINDOWS_MEAN_AND_LEAN
#include <windows.h>
#elif defined(IL_THREAD_SAFE)
#error "IL_THREAD_SAFE defined but don't know how to"
#endif

///////////////////////////////////////////////////////////////////////////
//
// Compiler specific definitions
//

#ifndef INLINE
    #if defined(__GNUC__)
        #define INLINE static inline
    #elif defined(_MSC_VER) 
        #define INLINE static __inline
    #else
        #define INLINE inline
    #endif
#endif


#ifndef NORETURN
    #if defined(__GNUC__) || defined(__clang__)
        #define NORETURN __attribute__((noreturn))
    #else
        #define NORETURN
    #endif
#endif

#if defined(_MSC_VER) || defined(__clang__)
  #define snprintf _snprintf
#endif

#define BIT(n)      (1<<n)
#define NUL         '\0'  // Easier to type and ?portable?
#define IL_PI       3.1415926535897932384626
#define IL_DEGCONV  0.0174532925199432957692

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable : 4756)  // Disables 'named type definition in parentheses' warning
#endif

//-----------------------------------------------
// Overflow handler for float-to-half conversion;
// generates a hardware floating-point overflow,
// which may be trapped by the operating system.
//-----------------------------------------------
INLINE ILfloat /*ILAPIENTRY*/ iFloatToHalfOverflow() {
  ILfloat f = 1e10;
  ILint j;
  for (j = 0; j < 10; j++)
    f *= f;       // this will overflow before
  // the for loop terminates
  return f;
}
#if defined(_MSC_VER)
  #pragma warning(pop)
#endif

//-----------------------------------------------------
// Float-to-half conversion -- general case, including
// zeroes, denormalized numbers and exponent overflows.
//-----------------------------------------------------
INLINE ILushort ILAPIENTRY iFloatToHalf(ILuint i) {
  //
  // Our floating point number, f, is represented by the bit
  // pattern in integer i.  Disassemble that bit pattern into
  // the sign, s, the exponent, e, and the significand, m.
  // Shift s into the position where it will go in in the
  // resulting half number.
  // Adjust e, accounting for the different exponent bias
  // of float and half (127 versus 15).
  //

  register int s =  (i >> 16) & 0x00008000;
  register int e = ((i >> 23) & 0x000000ff) - (127 - 15);
  register int m =   i        & 0x007fffff;

  //
  // Now reassemble s, e and m into a half:
  //

  if (e <= 0)
  {
    if (e < -10)
    {
      //
      // E is less than -10.  The absolute value of f is
      // less than HALF_MIN (f may be a small normalized
      // float, a denormalized float or a zero).
      //
      // We convert f to a half zero.
      //

      return 0;
    }

    //
    // E is between -10 and 0.  F is a normalized float,
    // whose magnitude is less than HALF_NRM_MIN.
    //
    // We convert f to a denormalized half.
    // 

    m = (m | 0x00800000) >> (1 - e);

    //
    // Round to nearest, round "0.5" up.
    //
    // Rounding may cause the significand to overflow and make
    // our number normalized.  Because of the way a half's bits
    // are laid out, we don't have to treat this case separately;
    // the code below will handle it correctly.
    // 

    if (m &  0x00001000)
      m += 0x00002000;

    //
    // Assemble the half from s, e (zero) and m.
    //

    return (ILushort)(s | (m >> 13));
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0)
    {
      //
      // F is an infinity; convert f to a half
      // infinity with the same sign as f.
      //

      return (ILushort)(s | 0x7c00);
    }
    else
    {
      //
      // F is a NAN; we produce a half NAN that preserves
      // the sign bit and the 10 leftmost bits of the
      // significand of f, with one exception: If the 10
      // leftmost bits are all zero, the NAN would turn 
      // into an infinity, so we have to set at least one
      // bit in the significand.
      //

      m >>= 13;
      return (ILushort)(s | 0x7c00 | m | (m == 0));
    }
  }
  else
  {
    //
    // E is greater than zero.  F is a normalized float.
    // We try to convert f to a normalized half.
    //

    //
    // Round to nearest, round "0.5" up
    //

    if (m &  0x00001000)
    {
      m += 0x00002000;

      if (m & 0x00800000)
      {
        m =  0;   // overflow in significand,
        e += 1;   // adjust exponent
      }
    }

    //
    // Handle exponent overflow
    //

    if (e > 30)
    {
      iFloatToHalfOverflow(); // Cause a hardware floating point overflow;
      return (ILushort)(s | 0x7c00);  // if this returns, the half becomes an
    }         // infinity with the same sign as f.

    //
    // Assemble the half from s, e and m.
    //

    return (ILushort)(s | (e << 10) | (m >> 13));
  }
}

// Taken from OpenEXR
INLINE ILuint ILAPIENTRY iHalfToFloat (ILushort y) {

  int s = (y >> 15) & 0x00000001;
  int e = (y >> 10) & 0x0000001f;
  int m =  y      & 0x000003ff;

  if (e == 0)
  {
    if (m == 0)
    {
      //
      // Plus or minus zero
      //

      return (ILuint)s << 31;
    }
    else
    {
      //
      // Denormalized number -- renormalize it
      //

      while (!(m & 0x00000400))
      {
        m <<= 1;
        e -=  1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0)
    {
      //
      // Positive or negative infinity
      //

      return (ILuint)(s << 31) | 0x7f800000;
    }
    else
    {
      //
      // Nan -- preserve sign and significand bits
      //

      return (ILuint)((s << 31) | 0x7f800000 | (m << 13));
    }
  }

  //
  // Normalized number
  //

  e = e + (127 - 15);
  m = m << 13;

  //
  // Assemble s, e and m.
  //

  return (ILuint)((s << 31) | (e << 23) | m);
}

INLINE void ILAPIENTRY iHalfToFloatV (const void *Half, void *Float) {
  *((ILuint*)Float) = iHalfToFloat(*((ILushort*)Half));
}

INLINE void ILAPIENTRY iFloatToHalfV (const void *Float, void *Half) {
  *((ILushort*)Half) = iFloatToHalf(*((ILuint*)Float));
}

INLINE ILfloat iGetHalf(const void *Half) {
  ILfloat temp;
  iHalfToFloatV(Half, &temp);
  return temp;
}

INLINE void iSetHalf(ILfloat f, void *Half) {
  iFloatToHalfV(&f, Half);
}

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
    fEofProc    eof;
    fGetcProc   getchar;
    fTellProc   tell;
    fPutcProc   putchar;
    fWriteProc  write;

    ILuint      lumpPos;
    ILHANDLE    handle;

    void *      lump;
    ILuint      lumpSize, lumpAlloc, ReadFileStart, WriteFileStart;
} SIO;

#define SIOopenWR(io,       f) ( (io)->handle = ((io)->openWrite    ? (io)->openWrite   (f) : NULL))
#define SIOclose( io         ) { if ((io)->close) (io)->close((io)->handle); (io)->handle = NULL; }
#define SIOread(  io, p, s, n) (io)->read   ((io)->handle, (p), (s), (n))
#define SIOseek(  io,    s, w) (io)->seek   ((io)->handle,      (s), (w))
#define SIOgetc(  io         ) (io)->getchar((io)->handle               )
#define SIOtell(  io         ) (io)->tell   ((io)->handle               )
#define SIOputc(  io,       c) (io)->putchar((c), (io)->handle          )
#define SIOwrite( io, p, s, n) (io)->write  ((p), (s), (n), (io)->handle)
#define SIOputs(  io,       s) SIOwrite(io, s, strlen(s), 1)
#define SIOfill(  io, v,    n) { ILuint i; for (i=0; i<n; i++) SIOputc((io), v); }
#define SIOpad(   io,       n) SIOfill(io, 0, n)

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
    io->lumpSize = ~0U;
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

typedef struct ILmeta {
  ILenum      IFD;
  ILenum      ID;
  ILuint      Type;
  ILuint      Length, Size;
  void *      Data;
  ILchar *    String;
  struct ILmeta *Next;
} ILmeta;

//! The Fundamental Image structure
/*! Every bit of information about an image is stored in this internal structure.
 * @internal
*/
typedef struct ILimage {
    ILuint          Width;       //!< the image's width
    ILuint          Height;      //!< the image's height
    ILuint          Depth;       //!< the image's depth
    ILubyte         Bpp;         //!< bytes per pixel (now number of channels)
    ILubyte         Bpc;         //!< bytes per channel
    ILushort        Extra;       //!< reserved
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
    ILuint          OffY;        //!< y-offset of the image
    ILubyte*        DxtcData;    //!< compressed data
    ILenum          DxtcFormat;  //!< compressed data format
    ILuint          DxtcSize;    //!< compressed data size
    SIO             io;

    struct ILimage* BaseImage;   //!< Top level image (for logging and metadata)
    ILmeta *        MetaTags;    //!< Metadata chain (only valid in base image)

#if IL_THREAD_SAFE_PTHREAD
    pthread_mutex_t Mutex;
#elif IL_THREAD_SAFE_WIN32
    HANDLE          Mutex;
#endif
} ILimage;

#define iBytes(ptr)    ((ILbyte*)  (void*)(ptr))
#define iShorts(ptr)   ((ILshort*) (void*)(ptr))
#define iInts(ptr)     ((ILint*)   (void*)(ptr))
#define iUBytes(ptr)   ((ILubyte*) (void*)(ptr))
#define iUShorts(ptr)  ((ILushort*)(void*)(ptr))
#define iUInts(ptr)    ((ILuint*)  (void*)(ptr))
#define iFloats(ptr)   ((ILfloat*) (void*)(ptr))
#define iDoubles(ptr)  ((ILdouble*)(void*)(ptr))

INLINE ILubyte  *iGetImageDataUByte(ILimage *img)   { return img->Data; }
INLINE ILushort *iGetImageDataUShort(ILimage *img)  { return iUShorts(img->Data); }
INLINE ILuint   *iGetImageDataUInt(ILimage *img)    { return iUInts(img->Data); }
INLINE ILfloat  *iGetImageDataFloat(ILimage *img)   { return iFloats(img->Data); }
INLINE ILdouble *iGetImageDataDouble(ILimage *img)  { return iDoubles(img->Data); }

INLINE void iCopyPixelElement( const void *Src, ILuint SrcIndex, void *Dest, ILuint DestIndex, ILuint Bpc ) {
  memcpy( iUBytes(Dest) + DestIndex * Bpc, iUBytes(Src) + SrcIndex * Bpc, Bpc );
}

INLINE ILfloat iGetPixelElement( const void *Buffer, ILuint Index, ILenum Type) {
  switch(Type) {
    case IL_UNSIGNED_BYTE:  return (iUBytes(Buffer) [Index]               ) / (ILfloat)IL_MAX_UNSIGNED_BYTE; 
    case IL_BYTE:           return ((iBytes(Buffer)  [Index] - IL_MIN_BYTE ) & IL_MAX_UNSIGNED_BYTE ) / (ILfloat)IL_MAX_UNSIGNED_BYTE;          
    case IL_UNSIGNED_SHORT: return (iUShorts(Buffer)[Index]               ) / (ILfloat)IL_MAX_UNSIGNED_SHORT;
    case IL_SHORT:          return ((iShorts(Buffer) [Index] - IL_MIN_SHORT) & IL_MAX_UNSIGNED_SHORT )/ (ILfloat)IL_MAX_UNSIGNED_SHORT;         
    case IL_UNSIGNED_INT:   return (iUInts(Buffer)  [Index]               ) / (ILfloat)IL_MAX_UNSIGNED_INT;
    case IL_INT:            return ((iInts(Buffer)   [Index] - IL_MIN_INT  ) & IL_MAX_UNSIGNED_INT ) / (ILfloat)IL_MAX_UNSIGNED_INT;           
    case IL_FLOAT:          return iFloats(Buffer)  [Index];
    case IL_DOUBLE:         return iDoubles(Buffer) [Index];
    case IL_HALF:           return iGetHalf( iShorts(Buffer)+Index );
  }
  return 0.0;
}

INLINE void iSetPixelElement( void *Buffer, ILuint Index, ILenum Type, ILdouble val ) {
  switch(Type) {
    case IL_UNSIGNED_BYTE:  iUBytes(Buffer) [Index] = val * IL_MAX_UNSIGNED_BYTE; break;
    case IL_BYTE:           iBytes(Buffer)  [Index] = val * IL_MAX_UNSIGNED_BYTE + IL_MIN_BYTE; break;
    case IL_UNSIGNED_SHORT: iUShorts(Buffer)[Index] = val * IL_MAX_UNSIGNED_SHORT; break;
    case IL_SHORT:          iShorts(Buffer) [Index] = val * IL_MAX_UNSIGNED_SHORT + IL_MIN_SHORT; break;
    case IL_UNSIGNED_INT:   iUInts(Buffer)  [Index] = val * IL_MAX_UNSIGNED_INT; break;
    case IL_INT:            iInts(Buffer)   [Index] = val * IL_MAX_UNSIGNED_INT + IL_MIN_INT; break;
    case IL_FLOAT:          iFloats(Buffer) [Index] = val; break;
    case IL_DOUBLE:         iDoubles(Buffer)[Index] = val; break;
    case IL_HALF:           iSetHalf( val, iShorts(Buffer)+Index ); break;
  }
}

INLINE void iSetPixelElementMax( void *Buffer, ILuint Index, ILenum Type ) {
  switch(Type) {
    case IL_UNSIGNED_BYTE:  iUBytes(Buffer) [Index] = IL_MAX_UNSIGNED_BYTE; break;
    case IL_BYTE:           iBytes(Buffer)  [Index] = IL_MAX_BYTE; break;
    case IL_UNSIGNED_SHORT: iUShorts(Buffer)[Index] = IL_MAX_UNSIGNED_SHORT; break;
    case IL_SHORT:          iShorts(Buffer) [Index] = IL_MAX_SHORT; break;
    case IL_UNSIGNED_INT:   iUInts(Buffer)  [Index] = IL_MAX_UNSIGNED_INT; break;
    case IL_INT:            iInts(Buffer)   [Index] = IL_MAX_INT; break;
    case IL_FLOAT:          iFloats(Buffer) [Index] = 1.0f; break;
    case IL_DOUBLE:         iDoubles(Buffer)[Index] = 1.0; break;
    case IL_HALF:           iSetHalf( 1.0f, iShorts(Buffer)+Index ); break;
  }
}

// Memory functions
ILAPI void*         ILAPIENTRY ialloc   (ILsizei Size);
ILAPI void          ILAPIENTRY ifreeReal(void *Ptr);
ILAPI void*         ILAPIENTRY icalloc  (const ILsizei Size, const ILsizei Num);

#define                    ifree(x) { ifreeReal(x); (x) = 0; }

/** Allocate an array of type @a T with @a n elements. */
#define                    iaalloc(T, n) (T*)icalloc(sizeof(T), (n))

/** Allocate one object of type @a T. */
#define                    ioalloc(T)    iaalloc(T, 1)

// Internal library functions in IL
ILAPI void      ILAPIENTRY iSetErrorReal(ILenum Error);
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
ILAPI ILubyte   ILAPIENTRY iGetBppFormat(ILenum Format);
ILAPI ILenum    ILAPIENTRY iGetFormatBpp(ILubyte Bpp);
ILAPI ILubyte   ILAPIENTRY iGetBpcType(ILenum Type);
ILAPI ILenum    ILAPIENTRY iGetTypeBpc(ILubyte Bpc);
ILAPI ILuint    ILAPIENTRY iGetMaxType(ILenum Type);
ILAPI ILubyte   ILAPIENTRY iGetBppPal(ILenum PalType);
ILAPI ILenum    ILAPIENTRY iGetPalBaseType(ILenum PalType);
ILAPI ILuint    ILAPIENTRY iGetMaskFormat(ILenum Format, ILubyte channel);
ILAPI ILboolean ILAPIENTRY iFormatHasAlpha(ILenum Format);

ILAPI ILuint    ILAPIENTRY iNextPower2(ILuint Num);
ILAPI ILenum    ILAPIENTRY iTypeFromExt(ILconst_string FileName);
ILAPI void      ILAPIENTRY iMemSwap(ILubyte *, ILubyte *, const ILuint);

ILAPI wchar_t * ILAPIENTRY iWideFromMultiByte(const char *Multi);
ILAPI char *    ILAPIENTRY iMultiByteFromWide(const wchar_t *Wide);

ILAPI ILboolean ILAPIENTRY iLoad(ILimage *Image, ILenum Type, ILconst_string FileName);
ILAPI void      ILAPIENTRY iSetInputLump(ILimage *, const void *Lump, ILuint Size);
ILAPI void      ILAPIENTRY iSetInputFile(ILimage *, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY iLoadFuncs2(ILimage *Image, ILenum Type);

ILAPI ILboolean ILAPIENTRY iSave(ILimage *Image, ILenum type, ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY iSaveImage(ILimage *Image, ILconst_string FileName);
ILAPI void      ILAPIENTRY iSetOutputLump(ILimage *, void *Lump, ILuint Size);
ILAPI void      ILAPIENTRY iSetOutputFile(ILimage *, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY iSaveFuncs2(ILimage* image, ILenum type);

//
// Image functions
//

ILAPI ILboolean ILAPIENTRY iClearImage      (ILimage *Image);
ILAPI ILboolean ILAPIENTRY iConvertImages   (ILimage *BaseImage, ILenum DestFormat, ILenum DestType);
ILAPI ILboolean ILAPIENTRY iCopyImage       (ILimage *DestImage, ILimage *SrcImage);
ILAPI ILboolean ILAPIENTRY iCopyImageAttr   (ILimage *Dest, ILimage *Src);
ILAPI ILboolean ILAPIENTRY iFlipImage       (ILimage *Image);
ILAPI ILboolean ILAPIENTRY iInitImage       (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY iMirrorImage     (ILimage *Image);
ILAPI ILboolean ILAPIENTRY iResizeImage     (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILboolean ILAPIENTRY iTexImage        (ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILimage * ILAPIENTRY iGetMipmap       (ILimage *Image, ILuint Number);
ILAPI ILimage * ILAPIENTRY iGetSubImage     (ILimage *Image, ILuint Number);
ILAPI ILimage*  ILAPIENTRY iCloneImage      (ILimage *Src);
ILAPI ILimage*  ILAPIENTRY iConvertImage    (ILimage *Image, ILenum DestFormat, ILenum DestType);
ILAPI ILimage*  ILAPIENTRY iNewImage        (ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc);
ILAPI ILimage*  ILAPIENTRY iNewImageFull    (ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, void *Data);
ILAPI ILuint    ILAPIENTRY iGetiv           (ILimage *Image, ILenum Mode, ILint *Param, ILint MaxCount);
ILAPI ILint     ILAPIENTRY iGetInteger      (ILimage *Image, ILenum Mode);
ILAPI ILuint    ILAPIENTRY iGetfv           (ILimage *Image, ILenum Mode, ILfloat *Param, ILint MaxCount);
ILAPI void      ILAPIENTRY iSetiv           (ILimage *Image, ILenum Mode, const ILint *Param);
ILAPI void      ILAPIENTRY iSetfv           (ILimage *Image, ILenum Mode, const ILfloat *Param);
ILAPI ILubyte*  ILAPIENTRY iGetFlipped      (ILimage *Image);
ILAPI ILuint    ILAPIENTRY iCopyPixels      (ILimage *Image, ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);
ILAPI ILuint    ILAPIENTRY iGetCurName      (void);
ILAPI ILuint    ILAPIENTRY iGetDXTCData     (ILimage *Image, void *Buffer, ILuint BufferSize, ILenum DXTCFormat);
ILAPI void      ILAPIENTRY iCloseImageReal  (ILimage *Image);
ILAPI void      ILAPIENTRY iFlipBuffer      (ILubyte *buff, ILuint depth, ILuint line_size, ILuint line_num);
ILAPI void      ILAPIENTRY iGetClear        (void *Colours, ILenum Format, ILenum Type);
ILAPI void*     ILAPIENTRY iConvertBuffer   (ILuint SizeOfData, ILenum SrcFormat, ILenum DestFormat, ILenum SrcType, ILenum DestType, ILpal *SrcPal, void *Buffer);

#define iCloseImage(img)    { iCloseImageReal(img); img = NULL; }

//
// Palette functions
//

ILAPI ILboolean ILAPIENTRY iConvertImagePal (ILimage *Image, ILenum DestFormat);
ILAPI ILboolean ILAPIENTRY iIsValidPal      (ILpal *Palette);
ILAPI ILpal*    ILAPIENTRY iConvertPal      (ILpal *Pal, ILenum DestFormat);
ILAPI ILpal*    ILAPIENTRY iCopyPalFromImage(ILimage *Image);
ILAPI void      ILAPIENTRY iClosePalReal    (ILpal *Palette);
#define iClosePal(pal)      { iClosePalReal(pal);   pal = NULL; }

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
#define iCharStrNCpy strncpy
#ifdef _WIN32
  #define iCharStrICmp stricmp
#else
  #define iCharStrICmp strcasecmp
#endif

ILAPI ILstring  ILAPIENTRY iStrDup(ILconst_string Str);
ILAPI char *    ILAPIENTRY iCharStrDup(const char *Str);
ILAPI ILstring  ILAPIENTRY iGetExtension(ILconst_string FileName);
//ILAPI ILboolean ILAPIENTRY iCheckExtension(ILconst_string Arg, ILconst_string Ext);

//
// ILU functions
//
ILAPI ILboolean ILAPIENTRY iSwapColours(ILimage *img);
ILAPI ILimage*  ILAPIENTRY iluRotate_(ILimage *Image, ILfloat Angle);
ILAPI ILimage*  ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle);
ILAPI ILimage*  ILAPIENTRY iluScale_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILenum Filter);
ILAPI ILboolean ILAPIENTRY iBuildMipmaps(ILimage *Parent, ILuint Width, ILuint Height, ILuint Depth);

#define imemclear(x,y) memset(x,0,y);

#if defined(_WIN32) && !defined(IL_STATIC_LIB)
  #if defined(_ILU_BUILD_LIBRARY) || defined(_ILUT_BUILD_LIBRARY)
    __declspec(dllimport)
  #else
    __declspec(dllexport) 
  #endif
#endif
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

#define iSetError(x) \
  { iTrace("**** Error: %04x", (x)); iSetErrorReal(x); }

#ifdef DEBUG
#define iAssert(x) { \
  bool __x = (x); \
  iTrace("Assertion failed: ", #x); \
  assert(x); \
}
#else
#define iAssert(x)
#endif

// check if p is completely inside buf
#define iCheckBuffer(p, buf, size) ( \
  ( (const ILubyte*)(p) >= (const ILubyte*)buf ) && \
  ( (const ILubyte*)(p) + sizeof(*(p)) <= (const ILubyte*)buf + size ) \
)

#ifdef __cplusplus
}
#endif

#endif//IL_EXPORTS_H
