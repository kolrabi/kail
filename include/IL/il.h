//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: IL/il.h
//
// Description: The main include file for DevIL
//
//-----------------------------------------------------------------------------

// Doxygen comment
/*! \file il.h
    The main include file for DevIL
*/

#ifndef __il_h_
#ifndef __IL_H__

#define __il_h_
#define __IL_H__

//this define controls if floats and doubles are clampled to [0..1]
//during conversion. It takes a little more time, but it is the correct
//way of doing this. If you are sure your floats are always valid,
//you can undefine this value...
#define CLAMP_HALF    1
#define CLAMP_FLOATS  1
#define CLAMP_DOUBLES 1

//
// IL-specific #define's
//

#define IL_VERSION_1_9_0
#define IL_VERSION_1_9_0
#define IL_VERSION_1_10_0
#define IL_VERSION              11000

#define IL_VARIANT_KAIL

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////
//
// Include required headers
//

#include <IL/config.h>
  
#include <stdio.h>
#include <limits.h>
#ifdef _UNICODE
  #ifndef _WIN32_WCE
    #include <wchar.h>
  #endif
#endif

#ifdef HAVE_STDINT_H
  #include <stdint.h>
#endif

///////////////////////////////////////////////////////////////////////////
//
// Compiler specific definitions
//

// import library automatically on compilers that support that
#ifdef _WIN32
  #if (defined(IL_USE_PRAGMA_LIBS)) && (!defined(_IL_BUILD_LIBRARY))
    #if defined(_MSC_VER) || defined(__BORLANDC__)
      #pragma comment(lib, "DevIL.lib")
    #endif
  #endif
#endif

// deal with compilers without restrict keyword
#ifdef RESTRICT_KEYWORD
  #define RESTRICT        restrict
  #define CONST_RESTRICT  const restrict
#else
  #define RESTRICT
  #define CONST_RESTRICT  const
#endif

// deprecation attribute
#if defined __GNUC__ && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0))
  // __attribute__((deprecated)) is supported by GCC 3.1 and later.
  #define IL_DEPRECATED(D) D __attribute__((deprecated))
#else
  #define IL_DEPRECATED(D) D
#endif

//
// Section shamelessly modified from the glut header.
//

// This is from Win32's <windef.h>
#if (defined(_MSC_VER) && _MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__) || defined(__LCC__)
  #define ILAPIENTRY            __stdcall 
//#elif defined(linux) || defined(MACOSX) || defined(__CYGWIN__) //fix bug 840364
#elif defined( __GNUC__ ) && defined(_WIN32)
  // this should work for any of the above commented platforms 
  // plus any platform using GCC
  #define ILAPIENTRY            __stdcall
#else
  #define ILAPIENTRY
#endif

// This is from Win32's <wingdi.h> and <winnt.h>
#if defined(__LCC__)
  #define ILAPI __stdcall
  #define ILAPI_DEPRECATED 
#elif defined(_WIN32) //changed 20031221 to fix bug 840421
  #ifdef IL_STATIC_LIB
    #define ILAPI
    #define ILAPI_DEPRECATED 
  #else
    #ifdef _IL_BUILD_LIBRARY
      #if defined(_MSC_VER) && _MSC_VER >= 1300
        // __declspec(deprecated) is supported by MSVC 7.0 and later.
        #define ILAPI_DEPRECATED __declspec(dllexport deprecated)
      #else
        #define ILAPI_DEPRECATED 
      #endif

      #define ILAPI __declspec(dllexport)
    #else
      #define ILAPI 
      #define ILAPI_DEPRECATED __declspec(deprecated)
    #endif
  #endif
#elif defined(__APPLE__)
  #define ILAPI extern
  #define ILAPI_DEPRECATED 
#else
  #define ILAPI
  #define ILAPI_DEPRECATED 
#endif

///////////////////////////////////////////////////////////////////////////
//
// Type definitions
// 

typedef unsigned int                ILenum;
typedef unsigned char               ILboolean;
typedef unsigned int                ILbitfield;
typedef signed char                 ILbyte;
typedef signed short                ILshort;
typedef int                         ILint;
typedef size_t                      ILsizei;
typedef unsigned char               ILubyte;
typedef unsigned short              ILushort;
typedef unsigned int                ILuint;
typedef float                       ILfloat;
typedef float                       ILclampf;
typedef double                      ILdouble;
typedef double                      ILclampd;

#ifdef _MSC_VER
  typedef __int64                   ILint64;
  typedef unsigned __int64          ILuint64;
#else
  #ifdef HAVE_STDINT_H
    typedef int64_t                 ILint64;
    typedef uint64_t                ILuint64;
  #else
    typedef long long int           ILint64;
    typedef long long unsigned int  ILuint64;
  #endif
#endif

// define character type
#ifdef _UNICODE
  typedef wchar_t                   ILchar;
  #define IL_TEXT(s)                L##s
  #ifdef _MSC_VER     
    #define IL_SFMT                 "%S"
  #else
    #define IL_SFMT                 "%ls"
  #endif
#else
  typedef char                      ILchar;
  #define IL_TEXT(s)                s
  #define IL_SFMT                   "%s"
#endif //_UNICODE

typedef ILchar *                    ILstring;
typedef ILchar const *              ILconst_string;

enum {
  IL_FALSE = 0,
  IL_TRUE  = 1
};

// limits
#define IL_MIN_BYTE                 SCHAR_MIN
#define IL_MIN_UNSIGNED_BYTE        0
#define IL_MIN_SHORT                SHRT_MIN
#define IL_MIN_UNSIGNED_SHORT       0
#define IL_MIN_INT                  INT_MIN
#define IL_MIN_UNSIGNED_INT         0

#define IL_MAX_BYTE                 SCHAR_MAX
#define IL_MAX_UNSIGNED_BYTE        UCHAR_MAX
#define IL_MAX_SHORT                SHRT_MAX
#define IL_MAX_UNSIGNED_SHORT       USHRT_MAX
#define IL_MAX_INT                  INT_MAX
#define IL_MAX_UNSIGNED_INT         UINT_MAX

///////////////////////////////////////////////////////////////////////////
//
// Useful macros
// 

#define IL_MAX(a,b)             (((a) > (b)) ? (a) : (b))
#define IL_MIN(a,b)             (((a) < (b)) ? (a) : (b))
#define IL_LIMIT(x,m,M)         ((x)<(m)?(m):((x)>(M)?(M):(x)))
#define IL_CLAMP(x)             IL_LIMIT((x),0,1)

///////////////////////////////////////////////////////////////////////////
//
// DevIL constants
// 

// Image data formats. Matches OpenGL's right now.
#define IL_COLOUR_INDEX         0x1900
#define IL_COLOR_INDEX          0x1900
#define IL_ALPHA                0x1906
#define IL_RGB                  0x1907
#define IL_RGBA                 0x1908
#define IL_BGR                  0x80E0
#define IL_BGRA                 0x80E1
#define IL_LUMINANCE            0x1909
#define IL_LUMINANCE_ALPHA      0x190A

// Image data types. Matches OpenGL's right now.
#define IL_BYTE                 0x1400
#define IL_UNSIGNED_BYTE        0x1401
#define IL_SHORT                0x1402
#define IL_UNSIGNED_SHORT       0x1403
#define IL_INT                  0x1404
#define IL_UNSIGNED_INT         0x1405
#define IL_FLOAT                0x1406
#define IL_DOUBLE               0x140A
#define IL_HALF                 0x140B

#define IL_VENDOR               0x1F00
#define IL_LOAD_EXT             0x1F01
#define IL_SAVE_EXT             0x1F02

// Attribute Bits
#define IL_ORIGIN_BIT           0x00000001
#define IL_FILE_BIT             0x00000002
#define IL_PAL_BIT              0x00000004
#define IL_FORMAT_BIT           0x00000008
#define IL_TYPE_BIT             0x00000010
#define IL_COMPRESS_BIT         0x00000020
#define IL_LOADFAIL_BIT         0x00000040
#define IL_FORMAT_SPECIFIC_BIT  0x00000080
#define IL_COLOUR_KEY_BIT       0x00000100 // 1.10
#define IL_CLEAR_COLOUR_BIT     0x00000200 // 1.10
#define IL_ALL_ATTRIB_BITS      0x000FFFFF

// Palette types
#define IL_PAL_NONE             0x0400
#define IL_PAL_RGB24            0x0401
#define IL_PAL_RGB32            0x0402
#define IL_PAL_RGBA32           0x0403
#define IL_PAL_BGR24            0x0404
#define IL_PAL_BGR32            0x0405
#define IL_PAL_BGRA32           0x0406

// Image types
#define IL_TYPE_UNKNOWN         0x0000
#define IL_BMP                  0x0420  //!< Microsoft Windows Bitmap - .bmp extension
#define IL_CUT                  0x0421  //!< Dr. Halo - .cut extension
#define IL_DOOM                 0x0422  //!< DooM walls - no specific extension
#define IL_DOOM_FLAT            0x0423  //!< DooM flats - no specific extension
#define IL_ICO                  0x0424  //!< Microsoft Windows Icons and Cursors - .ico and .cur extensions
#define IL_JPG                  0x0425  //!< JPEG - .jpg, .jpe and .jpeg extensions
#define IL_JFIF                 0x0425  //!< 
#define IL_ILBM                 0x0426  //!< Amiga IFF (FORM ILBM) - .iff, .ilbm, .lbm extensions
#define IL_PCD                  0x0427  //!< Kodak PhotoCD - .pcd extension
#define IL_PCX                  0x0428  //!< ZSoft PCX - .pcx extension
#define IL_PIC                  0x0429  //!< PIC - .pic extension
#define IL_PNG                  0x042A  //!< Portable Network Graphics - .png extension

#define IL_SGI                  0x042C  //!< Silicon Graphics - .sgi, .bw, .rgb and .rgba extensions
#define IL_TGA                  0x042D  //!< TrueVision Targa File - .tga, .vda, .icb and .vst extensions
#define IL_TIF                  0x042E  //!< Tagged Image File Format - .tif and .tiff extensions
#define IL_CHEAD                0x042F  //!< C-Style Header - .h extension
#define IL_RAW                  0x0430  //!< Raw Image Data - any extension
#define IL_MDL                  0x0431  //!< Half-Life Model Texture - .mdl extension
#define IL_WAL                  0x0432  //!< Quake 2 Texture - .wal extension
#define IL_LIF                  0x0434  //!< Homeworld Texture - .lif extension
#define IL_MNG                  0x0435  //!< Multiple-image Network Graphics - .mng extension
#define IL_JNG                  0x0435  //!< 
#define IL_GIF                  0x0436  //!< Graphics Interchange Format - .gif extension
#define IL_DDS                  0x0437  //!< DirectDraw Surface - .dds extension
#define IL_DCX                  0x0438  //!< ZSoft Multi-PCX - .dcx extension
#define IL_PSD                  0x0439  //!< Adobe PhotoShop - .psd extension
#define IL_EXIF                 0x043A  //!< 
#define IL_PSP                  0x043B  //!< PaintShop Pro - .psp extension
#define IL_PIX                  0x043C  //!< PIX - .pix extension
#define IL_PXR                  0x043D  //!< Pixar - .pxr extension
#define IL_XPM                  0x043E  //!< X Pixel Map - .xpm extension
#define IL_HDR                  0x043F  //!< Radiance High Dynamic Range - .hdr extension
#define IL_ICNS                 0x0440  //!< Macintosh Icon - .icns extension
#define IL_JP2                  0x0441  //!< Jpeg 2000 - .jp2 extension
#define IL_EXR                  0x0442  //!< OpenEXR - .exr extension
#define IL_WDP                  0x0443  //!< Microsoft HD Photo - .wdp and .hdp extension
#define IL_VTF                  0x0444  //!< Valve Texture Format - .vtf extension
#define IL_WBMP                 0x0445  //!< Wireless Bitmap - .wbmp extension
#define IL_SUN                  0x0446  //!< Sun Raster - .sun, .ras, .rs, .im1, .im8, .im24 and .im32 extensions
#define IL_IFF                  0x0447  //!< Interchange File Format - .iff extension
#define IL_TPL                  0x0448  //!< Gamecube Texture - .tpl extension
#define IL_FITS                 0x0449  //!< Flexible Image Transport System - .fit and .fits extensions
#define IL_DICOM                0x044A  //!< Digital Imaging and Communications in Medicine (DICOM) - .dcm and .dicom extensions
#define IL_IWI                  0x044B  //!< Call of Duty Infinity Ward Image - .iwi extension
#define IL_BLP                  0x044C  //!< Blizzard Texture Format - .blp extension
#define IL_FTX                  0x044D  //!< Heavy Metal: FAKK2 Texture - .ftx extension
#define IL_ROT                  0x044E  //!< Homeworld 2 - Relic Texture - .rot extension
#define IL_TEXTURE              0x044F  //!< Medieval II: Total War Texture - .texture extension
#define IL_DPX                  0x0450  //!< Digital Picture Exchange - .dpx extension
#define IL_UTX                  0x0451  //!< Unreal (and Unreal Tournament) Texture - .utx extension
#define IL_MP3                  0x0452  //!< MPEG-1 Audio Layer 3 - .mp3 extension

#define IL_PNM                  IL_PNM_PPM
#define IL_PNM_PPM              0x042B  //!< Portable Pix Map - .ppm extension
#define IL_PNM_PGM              0x0453  //!< Portable Gray Map - .pgm extension
#define IL_PNM_PBM              0x0454  //!< Portable Bit Map - .pbm extension

// Palette file format types
#define IL_JASC_PAL             0x0475  //!< PaintShop Pro Palette
#define IL_HALO_PAL             0x0476  //!< Dr. Halo Palette
#define IL_ACT_PAL              0x0477
#define IL_COL_PAL              0x0478
#define IL_PLT_PAL              0x0479

// Internal Error Types 
#define IL_NO_ERROR             0x0000

#define IL_INVALID_ENUM         0x0501
#define IL_OUT_OF_MEMORY        0x0502
#define IL_FORMAT_NOT_SUPPORTED 0x0503
#define IL_INTERNAL_ERROR       0x0504
#define IL_INVALID_VALUE        0x0505
#define IL_ILLEGAL_OPERATION    0x0506
#define IL_ILLEGAL_FILE_VALUE   0x0507
#define IL_INVALID_FILE_HEADER  0x0508
#define IL_INVALID_PARAM        0x0509
#define IL_COULD_NOT_OPEN_FILE  0x050A
#define IL_INVALID_EXTENSION    0x050B
#define IL_FILE_ALREADY_EXISTS  0x050C
#define IL_OUT_FORMAT_SAME      0x050D
#define IL_STACK_OVERFLOW       0x050E
#define IL_STACK_UNDERFLOW      0x050F
#define IL_INVALID_CONVERSION   0x0510
#define IL_BAD_DIMENSIONS       0x0511
#define IL_FILE_READ_ERROR      0x0512
#define IL_FILE_WRITE_ERROR     0x0512

// Library Error Types
#define IL_LIB_GIF_ERROR        0x05E1 // unused
#define IL_LIB_JPEG_ERROR       0x05E2
#define IL_LIB_PNG_ERROR        0x05E3
#define IL_LIB_TIFF_ERROR       0x05E4
#define IL_LIB_MNG_ERROR        0x05E5
#define IL_LIB_JP2_ERROR        0x05E6
#define IL_LIB_EXR_ERROR        0x05E7
#define IL_UNKNOWN_ERROR        0x05FF

// Origin Definitions
#define IL_ORIGIN_SET           0x0600
#define IL_ORIGIN_LOWER_LEFT    0x0601
#define IL_ORIGIN_UPPER_LEFT    0x0602
#define IL_ORIGIN_MODE          0x0603

// Format and Type Mode Definitions
#define IL_FORMAT_SET           0x0610
#define IL_FORMAT_MODE          0x0611
#define IL_TYPE_SET             0x0612
#define IL_TYPE_MODE            0x0613

// File definitions
#define IL_FILE_OVERWRITE       0x0602 // deprectated: value not used anywhere, use IL_FILE_MODE instead
#define IL_FILE_MODE            0x0621

// Palette definitions
#define IL_CONV_PAL             0x0630

// Load fail definitions
#define IL_DEFAULT_ON_FAIL      0x0632

// Key colour and alpha definitions
#define IL_USE_KEY_COLOUR       0x0635
#define IL_USE_KEY_COLOR        0x0635
#define IL_BLIT_BLEND           0x0636

// Interlace definitions
#define IL_SAVE_INTERLACED      0x0639 // deprecated: not used, use IL_PNG_INTERLACE etc.
#define IL_INTERLACE_MODE       0x063A // deprecated: not used, use IL_PNG_INTERLACE etc.

// Quantization definitions
#define IL_QUANTIZATION_MODE    0x0640
#define IL_WU_QUANT             0x0641
#define IL_NEU_QUANT            0x0642
#define IL_NEU_QUANT_SAMPLE     0x0643
#define IL_MAX_QUANT_INDEXS     0x0644 //XIX : ILint : Maximum number of colors to reduce to, default of 256. and has a range of 2-256
#define IL_MAX_QUANT_INDICES    0x0644 // Redefined, since the above #define is misspelled

// Hints
#define IL_FASTEST              0x0660
#define IL_LESS_MEM             0x0661
#define IL_DONT_CARE            0x0662
#define IL_MEM_SPEED_HINT       0x0665
#define IL_USE_COMPRESSION      0x0666
#define IL_NO_COMPRESSION       0x0667
#define IL_COMPRESSION_HINT     0x0668

// Compression
#define IL_NVIDIA_COMPRESS      0x0670 // deprecated: nvidia texture tools now use libsquish anyway
#define IL_SQUISH_COMPRESS      0x0671

// Subimage types
#define IL_SUB_NEXT             0x0680
#define IL_SUB_MIPMAP           0x0681
#define IL_SUB_LAYER            0x0682

// Compression definitions
#define IL_COMPRESS_MODE        0x0700
#define IL_COMPRESS_NONE        0x0701
#define IL_COMPRESS_RLE         0x0702
#define IL_COMPRESS_LZO         0x0703
#define IL_COMPRESS_ZLIB        0x0704

// File format-specific values
#define IL_TGA_CREATE_STAMP        0x0710 // deprecated: not used anywhere
#define IL_JPG_QUALITY             0x0711
#define IL_PNG_INTERLACE           0x0712
#define IL_TGA_RLE                 0x0713
#define IL_BMP_RLE                 0x0714
#define IL_SGI_RLE                 0x0715

#define IL_TGA_ID_STRING           0x0717
#define IL_TGA_AUTHNAME_STRING     0x0718
#define IL_TGA_AUTHCOMMENT_STRING  0x0719
#define IL_PNG_AUTHNAME_STRING     0x071A
#define IL_PNG_TITLE_STRING        0x071B
#define IL_PNG_DESCRIPTION_STRING  0x071C
#define IL_TIF_DESCRIPTION_STRING  0x071D
#define IL_TIF_HOSTCOMPUTER_STRING 0x071E
#define IL_TIF_DOCUMENTNAME_STRING 0x071F
#define IL_TIF_AUTHNAME_STRING     0x0720

#define IL_JPG_SAVE_FORMAT         0x0721
#define IL_CHEAD_HEADER_STRING     0x0722
#define IL_PCD_PICNUM              0x0723
#define IL_PNG_ALPHA_INDEX         0x0724 //XIX : ILint : the color in the palette at this index value (0-255) is considered transparent, -1 for no trasparent color
#define IL_JPG_PROGRESSIVE         0x0725
#define IL_VTF_COMP                0x0726

// DXTC definitions
#define IL_DXTC_FORMAT          0x0705
#define IL_DXT1                 0x0706
#define IL_DXT2                 0x0707
#define IL_DXT3                 0x0708
#define IL_DXT4                 0x0709
#define IL_DXT5                 0x070A
#define IL_DXT_NO_COMP          0x070B
#define IL_KEEP_DXTC_DATA       0x070C
#define IL_DXTC_DATA_FORMAT     0x070D
#define IL_3DC                  0x070E
#define IL_RXGB                 0x070F
#define IL_ATI1N                0x0710
#define IL_DXT1A                0x0711  // Normally the same as IL_DXT1, except for nVidia Texture Tools.

// Environment map definitions
#define IL_CUBEMAP_POSITIVEX    0x00000400
#define IL_CUBEMAP_NEGATIVEX    0x00000800
#define IL_CUBEMAP_POSITIVEY    0x00001000
#define IL_CUBEMAP_NEGATIVEY    0x00002000
#define IL_CUBEMAP_POSITIVEZ    0x00004000
#define IL_CUBEMAP_NEGATIVEZ    0x00008000
#define IL_SPHEREMAP            0x00010000

// Values
#define IL_VERSION_NUM           0x0DE2

#define IL_IMAGE_WIDTH           0x0DE4
#define IL_IMAGE_HEIGHT          0x0DE5
#define IL_IMAGE_DEPTH           0x0DE6
#define IL_IMAGE_SIZE_OF_DATA    0x0DE7
#define IL_IMAGE_BPP             0x0DE8
#define IL_IMAGE_BYTES_PER_PIXEL 0x0DE8
#define IL_IMAGE_BPP             0x0DE8
#define IL_IMAGE_BITS_PER_PIXEL  0x0DE9
#define IL_IMAGE_FORMAT          0x0DEA
#define IL_IMAGE_TYPE            0x0DEB

#define IL_PALETTE_TYPE          0x0DEC
#define IL_PALETTE_SIZE          0x0DED
#define IL_PALETTE_BPP           0x0DEE
#define IL_PALETTE_NUM_COLS      0x0DEF
#define IL_PALETTE_BASE_TYPE     0x0DF0

#define IL_NUM_FACES             0x0DE1
#define IL_NUM_IMAGES            0x0DF1
#define IL_NUM_MIPMAPS           0x0DF2
#define IL_NUM_LAYERS            0x0DF3

#define IL_ACTIVE_IMAGE          0x0DF4
#define IL_ACTIVE_MIPMAP         0x0DF5
#define IL_ACTIVE_LAYER          0x0DF6
#define IL_ACTIVE_FACE           0x0E00

#define IL_CUR_IMAGE             0x0DF7

#define IL_IMAGE_DURATION        0x0DF8
#define IL_IMAGE_PLANESIZE       0x0DF9
#define IL_IMAGE_BPC             0x0DFA
#define IL_IMAGE_OFFX            0x0DFB
#define IL_IMAGE_OFFY            0x0DFC
#define IL_IMAGE_CUBEFLAGS       0x0DFD
#define IL_IMAGE_ORIGIN          0x0DFE
#define IL_IMAGE_CHANNELS        0x0DFF

// kaIL 1.10.0
#define IL_IMAGE_SELECTION_MODE     0x0650
#define IL_RELATIVE                 0x0651
#define IL_ABSOLUTE                 0x0652
#define IL_IMAGE_METADATA_COUNT     0x0E01

//
// Exif
//

// Metadata strings
#define IL_META_MAKE                          0x10000
#define IL_META_MODEL                         0x10001
#define IL_META_DOCUMENT_NAME                 0x10002
#define IL_META_IMAGE_DESCRIPTION             0x10003
#define IL_META_SOFTWARE                      0x10004
#define IL_META_DATETIME                      0x10005
#define IL_META_ARTIST                        0x10006
#define IL_META_HOST_COMPUTER                 0x10007
#define IL_META_COPYRIGHT                     0x10008
#define IL_META_SPECTRAL_SENSITIVITY          0x10009
#define IL_META_DATETIME_ORIGINAL             0x1000A
#define IL_META_DATETIME_DIGITIZED            0x1000B
#define IL_META_SUB_SECOND_TIME               0x1000C
#define IL_META_SUB_SECOND_TIME_ORIGINAL      0x1000D
#define IL_META_SUB_SECOND_TIME_DIGITIZED     0x1000E
#define IL_META_EXIF_VERSION                  0x1000F // 0220 -> 2.20

#define IL_META_COMPRESSION                   0x10010
#define IL_META_RESOLUTION_UNIT               0x10011

#define IL_META_GPS_LATITUDE_REF              0x11000 // N / S
#define IL_META_GPS_LONGITUDE_REF             0x11001 // E / W
#define IL_META_GPS_SATELLITES                0x11002
#define IL_META_GPS_STATUS                    0x11003 // A: in progress, V: interoperability
#define IL_META_GPS_MEASURE_MODE              0x11004 // 2: 2d, 3: 3d
#define IL_META_GPS_SPEED_REF                 0x11005 // K: km/h, M: mp/h, N: knots
#define IL_META_GPS_TRACK_REF                 0x11006 // T: true, M: magnetic
#define IL_META_GPS_IMAGE_DIRECTION_REF       0x11007 // T: true, M: magnetic
#define IL_META_GPS_MAP_DATUM                 0x11008
#define IL_META_GPS_DEST_LATITUDE_REF         0x11009 // N / S
#define IL_META_GPS_DEST_LONGITUDE_REF        0x1100A // E / W
#define IL_META_GPS_DEST_BEARING_REF          0x1100B // T: true, M: magnetic
#define IL_META_GPS_DEST_DISTANCE_REF         0x1100C // K: km, M: miles, N: knots
#define IL_META_GPS_DATESTAMP                 0x1100D // 

// Metadata integers
#define IL_META_PAGE_NUMBER                   0x20000

#define IL_META_EXPOSURE_TIME                 0x20001 // 2 ints (1 rational)
#define IL_META_FSTOP                         0x20002 // 2 ints (1 rational)
#define IL_META_EXPOSURE_PROGRAM              0x20003
#define IL_META_SHUTTER_SPEED                 0x20004 // 2 ints (1 rational)
#define IL_META_ISO_RATINGS                   0x20005 // variable number of ints

#define IL_META_APERTURE                      0x20007 // 2 ints (1 rational)
#define IL_META_BRIGHTNESS                    0x20008 // 2 ints (1 signed rational)
#define IL_META_EXPOSURE_BIAS                 0x20009 // 2 ints (1 signed rational)
#define IL_META_MAX_APERTURE                  0x2000A // 2 ints (1 rational)
#define IL_META_SUBJECT_DISTANCE              0x2000B // 2 ints (1 rational)
#define IL_META_METERING_MODE                 0x2000C
#define IL_META_LIGHT_SOURCE                  0x2000D
#define IL_META_FLASH                         0x2000E 
#define IL_META_FOCAL_LENGTH                  0x2000F // 2 ints (1 rational)
#define IL_META_FLASH_ENERGY                  0x20010 // 2 ints (1 rational)
#define IL_META_SUBJECT_AREA                  0x20011 // 2-3 ints

#define IL_META_X_RESOLUTION                  0x20012 // 2 ints (1 rational)
#define IL_META_Y_RESOLUTION                  0x20013 // 2 ints (1 rational)
#define IL_META_COLOUR_SPACE                  0x20014
#define IL_META_COLOR_SPACE                   0x20014
#define IL_META_EXPOSURE_MODE                 0x20015
#define IL_META_WHITE_BALANCE                 0x20016
#define IL_META_SENSING_METHOD                0x20017

#define IL_META_GPS_VERSION                   0x21000
#define IL_META_GPS_LATITUDE                  0x21001 // 6 ints (3 rationals: DMS)
#define IL_META_GPS_LONGITUDE                 0x21002 // 6 ints (3 rationals: DMS)
#define IL_META_GPS_ALTITUDE_REF              0x21003 // 0: above sea leve, 1: below
#define IL_META_GPS_ALTITUDE                  0x21004 // 2 ints (1 rational)
#define IL_META_GPS_TIMESTAMP                 0x21005 // 6 ints (3 rationals: HMS)
#define IL_META_GPS_DOP                       0x21006 // 2 ints (1 rational)
#define IL_META_GPS_SPEED                     0x21007 // 2 ints (1 rational)
#define IL_META_GPS_TRACK                     0x21008 // 2 ints (1 rational)
#define IL_META_GPS_IMAGE_DIRECTION           0x21009 // 2 ints (1 rational)
#define IL_META_GPS_DEST_LATITUDE             0x2100A // 6 ints (3 rationals: DMS)
#define IL_META_GPS_DEST_LONGITUDE            0x2100B // 6 ints (3 rationals: DMS)
#define IL_META_GPS_DEST_BEARING              0x2100C // 2 ints (1 rational)
#define IL_META_GPS_DEST_DISTANCE             0x2100D // 2 ints (1 rational)
#define IL_META_GPS_DIFFERENTIAL              0x2100E // 1 int

// blobs
#define IL_META_MAKER_NOTE                    0x30000
#define IL_META_FLASHPIX_VERSION              0x30001
#define IL_META_FILESOURCE                    0x30002
#define IL_META_USER_COMMENT                  0x30003

#define IL_EXPOSURE_PROGRAM_NOT_DEFINED       0
#define IL_EXPOSURE_PROGRAM_MANUAL            1
#define IL_EXPOSURE_PROGRAM_NORMAL            2
#define IL_EXPOSURE_PROGRAM_APERTURE_PRIORITY 3
#define IL_EXPOSURE_PROGRAM_SHUTTER_PRIORITY  4
#define IL_EXPOSURE_PROGRAM_CREATIVE          5
#define IL_EXPOSURE_PROGRAM_ACTION            6
#define IL_EXPOSURE_PROGRAM_PORTRAIT          7
#define IL_EXPOSURE_PROGRAM_LANDSCAPE         8

#define IL_METERING_MODE_UNKNOWN              0
#define IL_METERING_MODE_AVERAGE              1
#define IL_METERING_MODE_CENTER_WEIGHTED      2
#define IL_METERING_MODE_SPOT                 3
#define IL_METERING_MODE_MULTI_SPOT           4
#define IL_METERING_MODE_PATTERN              5
#define IL_METERING_MODE_PARTIAL              6
#define IL_METERING_MODE_OTHER                255

#define IL_LIGHT_SOURCE_UNKNOWN               0
#define IL_LIGHT_SOURCE_DAYLIGHT              1 
#define IL_LIGHT_SOURCE_FLUORESCENT           2
#define IL_LIGHT_SOURCE_TUNGSTEN              3
#define IL_LIGHT_SOURCE_FLASH                 4
#define IL_LIGHT_SOURCE_FINE_WEATHER          9
#define IL_LIGHT_SOURCE_CLOUDY_WEATHER        10
#define IL_LIGHT_SOURCE_SHADE                 11
#define IL_LIGHT_SOURCE_DAYLIGHT_FLUORESCENT  12
#define IL_LIGHT_SOURCE_DAY_WHITE_FLUORESCENT 13
#define IL_LIGHT_SOURCE_COOL_WHITE_FLUORESCENT 14
#define IL_LIGHT_SOURCE_WHITE_FLUORESCENT     15
#define IL_LIGHT_SOURCE_STANDARD_LIGHT_A      17
#define IL_LIGHT_SOURCE_STANDARD_LIGHT_B      18
#define IL_LIGHT_SOURCE_STANDARD_LIGHT_C      19
#define IL_LIGHT_SOURCE_D55                   20
#define IL_LIGHT_SOURCE_D65                   21
#define IL_LIGHT_SOURCE_D75                   22
#define IL_LIGHT_SOURCE_D50                   23
#define IL_LIGHT_SOURCE_ISO_TUNGSTEN          24
#define IL_LIGHT_SOURCE_OTHER                 255

#define IL_FLASH_FIRED                        ( 1 << 0 )
#define IL_FLASH_STROBE_RETURN_UNSUPPORTED    ( 2 << 1 )
#define IL_FLASH_STROBE_RETURN_NOT_DETECTED   ( 2 << 1 )
#define IL_FLASH_STROBE_RETURN_DETECTED       ( 3 << 1 )
#define IL_FLASH_COMPULSORY_FIRING            ( 1 << 3 )
#define IL_FLASH_COMPULSORY_SUPPRESSION       ( 2 << 3 )
#define IL_FLASH_AUTO                         ( 3 << 3 )
#define IL_FLASH_SUPPORTED                    ( 1 << 5 )
#define IL_FLASH_RED_EYE_REDUCTION            ( 1 << 6 )

// not (yet) supported
// #define IL_META_OECF                      0x8828
// #define IL_META_COMPONENTS_CONFIGURATION  0x9101
// #define IL_META_SUBJECT_AREA              0x9214
// #define IL_TAG_EXIF_FLASHPIX_VERSION          0xA000
// #define IL_TAG_EXIF_PIXEL_X_DIMENSION         0xA002
// #define IL_TAG_EXIF_PIXEL_Y_DIMENSION         0xA003
// #define IL_TAG_EXIF_RELATED_SOUND_FILE        0xA004

// #define IL_TAG_EXIF_SPATIAL_FREQUENCY_RESPONSE 0xA20C
// #define IL_TAG_EXIF_FOCAL_PLANE_X_RESOLUTION  0xA20E
// #define IL_TAG_EXIF_FOCAL_PLANE_Y_RESOLUTION  0xA20F
// #define IL_TAG_EXIF_FOCAL_PLANE_RESOULTION_UNIT 0xA210
// 
// #define IL_TAG_EXIF_SUBJECT_LOCATION          0xA214
// #define IL_TAG_EXIF_EXPOSURE_INDEX            0xA215
// #define IL_TAG_EXIF_FILE_SOURCE               0xA300
// #define IL_TAG_EXIF_SCENE_TYPE                0xA301
// #define IL_TAG_EXIF_CFA_PATTERN               0xA302
// #define IL_TAG_EXIF_SCENE_CAPTURE_TYPE        0xA406
// #define IL_TAG_EXIF_GAIN_CONTROL              0xA407
// #define IL_TAG_EXIF_CONTRAST                  0xA408
// #define IL_TAG_EXIF_SATURATION                0xA409
// #define IL_TAG_EXIF_SHARPNESS                 0xA40A
// #define IL_TAG_EXIF_DEVICE_SETTING_DESCRIPTION 0xA40B
// #define IL_TAG_EXIF_SUBJECT_DISTANCE_RANGE    0xA40C
// #define IL_TAG_EXIF_IMAGE_UNIQUE_ID           0xA420


// Tag data types
#define IL_EXIF_TYPE_NONE     0
#define IL_EXIF_TYPE_BYTE     1
#define IL_EXIF_TYPE_ASCII    2
#define IL_EXIF_TYPE_WORD     3
#define IL_EXIF_TYPE_DWORD    4
#define IL_EXIF_TYPE_RATIONAL 5
#define IL_EXIF_TYPE_SBYTE    6
#define IL_EXIF_TYPE_BLOB     7
#define IL_EXIF_TYPE_SWORD    8
#define IL_EXIF_TYPE_SDWORD   9
#define IL_EXIF_TYPE_SRATIONAL 10
#define IL_EXIF_TYPE_FLOAT    11
#define IL_EXIF_TYPE_DOUBLE   12
#define IL_EXIF_TYPE_IFD      13

// TIFF IFDs
#define IL_TIFF_IFD0                          0x0000
#define IL_TIFF_IFD1                          0x0001
#define IL_TIFF_IFD_EXIF                      0x8769
#define IL_TIFF_IFD_GPS                       0x8825
#define IL_TIFF_IFD_INTEROP                   0xA005

///////////////////////////////////////////////////////////////////////////
//
// IO section
//

#define IL_SEEK_SET             0
#define IL_SEEK_CUR             1
#define IL_SEEK_END             2

#define IL_EOF                  -1

// Callback functions for file reading and writing
typedef void*      ILHANDLE;
typedef void      (ILAPIENTRY *fCloseProc)  (ILHANDLE);
typedef ILboolean (ILAPIENTRY *fEofProc)    (ILHANDLE);
typedef ILint     (ILAPIENTRY *fGetcProc)   (ILHANDLE);
typedef ILHANDLE  (ILAPIENTRY *fOpenProc)   (ILconst_string);
typedef ILuint    (ILAPIENTRY *fReadProc)   (ILHANDLE, void*, ILuint, ILuint);
typedef ILint     (ILAPIENTRY *fSeekProc)   (ILHANDLE, ILint64, ILuint);
typedef ILuint    (ILAPIENTRY *fTellProc)   (ILHANDLE);
typedef ILint     (ILAPIENTRY *fPutcProc)   (ILubyte, ILHANDLE);
typedef ILuint    (ILAPIENTRY *fWriteProc)  (const void*, ILuint, ILuint, ILHANDLE); // FIXME

// Callback functions for allocation and deallocation
typedef void*     (ILAPIENTRY *mAlloc)      (ILsizei);
typedef void      (ILAPIENTRY *mFree)       (void*);

// Registered format procedures
typedef ILenum    (ILAPIENTRY *IL_LOADPROC) (ILconst_string);
typedef ILenum    (ILAPIENTRY *IL_SAVEPROC) (ILconst_string);

///////////////////////////////////////////////////////////////////////////
//
// ImageLib Functions
// 

// State
ILAPI void      ILAPIENTRY ilBindImage(ILuint Image);
ILAPI_DEPRECATED ILboolean ILAPIENTRY IL_DEPRECATED(ilCompressFunc(ILenum Mode)); // value not used anywhere
ILAPI ILboolean ILAPIENTRY ilDisable(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilEnable(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilFormatFunc(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilGetBoolean(ILenum Mode);
ILAPI void      ILAPIENTRY ilGetBooleanv(ILenum Mode, ILboolean *Param);
ILAPI ILenum    ILAPIENTRY ilGetError(void);
ILAPI ILint     ILAPIENTRY ilGetInteger(ILenum Mode);
ILAPI ILuint    ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param);
ILAPI ILconst_string  ILAPIENTRY ilGetString(ILenum StringName);
ILAPI void      ILAPIENTRY ilHint(ILenum Target, ILenum Mode);
ILAPI void      ILAPIENTRY ilInit(void);
ILAPI ILboolean ILAPIENTRY ilIsDisabled(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilIsEnabled(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilIsImage(ILuint Image);
ILAPI void      ILAPIENTRY ilSetInteger(ILenum Mode, ILint Param);
ILAPI void      ILAPIENTRY ilSetMemory(mAlloc, mFree);
ILAPI void      ILAPIENTRY ilSetString(ILenum Mode, ILconst_string String);
ILAPI void      ILAPIENTRY ilShutDown(void);
ILAPI void      ILAPIENTRY IL_DEPRECATED(ilResetMemory(void));

// Image management
ILAPI ILuint    ILAPIENTRY ilCloneCurImage(void);
ILAPI ILboolean ILAPIENTRY ilCopyImage(ILuint Src);
ILAPI ILuint    ILAPIENTRY ilCopyPixels(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);
ILAPI ILuint    ILAPIENTRY ilCreateSubImage(ILenum Type, ILuint Num);
ILAPI void      ILAPIENTRY ilDeleteImage(const ILuint Num);
ILAPI void      ILAPIENTRY ilDeleteImages(ILsizei Num, const ILuint *Images);
ILAPI ILuint    ILAPIENTRY ilGenImage(void);
ILAPI void      ILAPIENTRY ilGenImages(ILsizei Num, ILuint *Images);
ILAPI void      ILAPIENTRY ilPopAttrib(void);
ILAPI void      ILAPIENTRY ilPushAttrib(ILuint Bits);
ILAPI void      ILAPIENTRY ilClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
ILAPI void      ILAPIENTRY ilKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha);
ILAPI ILboolean ILAPIENTRY ilOriginFunc(ILenum Mode);
ILAPI ILboolean ILAPIENTRY ilTypeFunc(ILenum Mode);

// Image selection
ILAPI ILboolean ILAPIENTRY ilActiveFace(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilActiveImage(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilActiveLayer(ILuint Number);
ILAPI ILboolean ILAPIENTRY ilActiveMipmap(ILuint Number);

// Image manipulation
ILAPI ILboolean ILAPIENTRY ilApplyPal(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilApplyProfile(ILstring InProfile, ILstring OutProfile);
ILAPI ILboolean ILAPIENTRY ilBlit(ILuint Source, ILint DestX, ILint DestY, ILint DestZ, ILuint SrcX, ILuint SrcY, ILuint SrcZ, ILuint Width, ILuint Height, ILuint Depth);
ILAPI ILboolean ILAPIENTRY ilClampNTSC(void);
ILAPI ILboolean ILAPIENTRY ilClearImage(void);
ILAPI ILboolean ILAPIENTRY ilConvertImage(ILenum DestFormat, ILenum DestType);
ILAPI ILboolean ILAPIENTRY ilConvertPal(ILenum DestFormat);
ILAPI ILboolean ILAPIENTRY ilDefaultImage(void);
ILAPI void      ILAPIENTRY ilModAlpha(ILdouble AlphaValue);
ILAPI ILboolean ILAPIENTRY ilOverlayImage(ILuint Source, ILint XCoord, ILint YCoord, ILint ZCoord);

// Image data
ILAPI ILubyte*  ILAPIENTRY ilCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DXTCFormat, ILuint *DXTCSize);
ILAPI ILboolean ILAPIENTRY ilDxtcDataToImage(void);
ILAPI ILboolean ILAPIENTRY ilDxtcDataToSurface(void);
ILAPI void      ILAPIENTRY ilFlipSurfaceDxtcData(void);
ILAPI ILubyte*  ILAPIENTRY ilGetAlpha(ILenum Type);
ILAPI ILubyte*  ILAPIENTRY ilGetData(void);
ILAPI ILuint    ILAPIENTRY ilGetDXTCData(void *Buffer, ILuint BufferSize, ILenum DXTCFormat);
ILAPI ILubyte*  ILAPIENTRY ilGetPalette(void);
ILAPI ILboolean ILAPIENTRY ilInvertSurfaceDxtcDataAlpha(void);
ILAPI ILboolean ILAPIENTRY ilImageToDxtcData(ILenum Format);
ILAPI ILboolean ILAPIENTRY ilSetAlpha(ILdouble AlphaValue);
ILAPI ILboolean ILAPIENTRY ilSetData(void *Data);
ILAPI ILboolean ILAPIENTRY ilSetDuration(ILuint Duration);
ILAPI void      ILAPIENTRY ilSetPixels(ILint XOff, ILint YOff, ILint ZOff, ILuint Width, ILuint Height, ILuint Depth, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilSurfaceToDxtcData(ILenum Format);
ILAPI ILboolean ILAPIENTRY ilTexImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte NumChannels, ILenum Format, ILenum Type, void *Data);
ILAPI ILboolean ILAPIENTRY ilTexImageDxtc(ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, const ILubyte* data);

// Image files
ILAPI ILuint    ILAPIENTRY ilDetermineSize(ILenum Type);
ILAPI ILenum    ILAPIENTRY ilDetermineType(ILconst_string FileName);
ILAPI ILenum    ILAPIENTRY ilDetermineTypeFuncs(void);
ILAPI ILenum    ILAPIENTRY ilDetermineTypeF(ILHANDLE File);
ILAPI ILenum    ILAPIENTRY ilDetermineTypeL(const void *Lump, ILuint Size);
ILAPI ILuint64  ILAPIENTRY ilGetLumpPos(void);

ILAPI ILboolean ILAPIENTRY ilIsValid(ILenum Type, ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilIsValidF(ILenum Type, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY ilIsValidL(ILenum Type, void *Lump, ILuint Size);

ILAPI ILboolean ILAPIENTRY ilLoad(ILenum Type, ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilLoadF(ILenum Type, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY ilLoadFuncs(ILenum Type);
ILAPI ILboolean ILAPIENTRY ilLoadImage(ILconst_string FileName);
ILAPI ILboolean ILAPIENTRY ilLoadL(ILenum Type, const void *Lump, ILuint Size);
ILAPI ILboolean ILAPIENTRY ilLoadPal(ILconst_string FileName);

ILAPI void      ILAPIENTRY ilRegisterFormat(ILenum Format);
ILAPI ILboolean ILAPIENTRY ilRegisterMipNum(ILuint Num);
ILAPI ILboolean ILAPIENTRY ilRegisterNumFaces(ILuint Num);
ILAPI ILboolean ILAPIENTRY ilRegisterNumImages(ILuint Num);
ILAPI void      ILAPIENTRY ilRegisterOrigin(ILenum Origin);
ILAPI void      ILAPIENTRY ilRegisterPal(void *Pal, ILuint Size, ILenum Type);
ILAPI void      ILAPIENTRY ilRegisterType(ILenum Type);

ILAPI ILboolean ILAPIENTRY ilRegisterLoad(ILconst_string Ext, IL_LOADPROC Load);
ILAPI ILboolean ILAPIENTRY ilRegisterSave(ILconst_string Ext, IL_SAVEPROC Save);
ILAPI ILboolean ILAPIENTRY ilRemoveLoad(ILconst_string Ext);
ILAPI ILboolean ILAPIENTRY ilRemoveSave(ILconst_string Ext);

ILAPI void      ILAPIENTRY ilResetRead(void);
ILAPI void      ILAPIENTRY ilResetWrite(void);

ILAPI ILboolean ILAPIENTRY ilSave(ILenum Type, ILconst_string FileName);
ILAPI ILuint    ILAPIENTRY ilSaveF(ILenum Type, ILHANDLE File);
ILAPI ILboolean ILAPIENTRY ilSaveFuncs(ILenum type);
ILAPI ILboolean ILAPIENTRY ilSaveImage(ILconst_string FileName);
ILAPI ILuint    ILAPIENTRY ilSaveL(ILenum Type, void *Lump, ILuint Size);
ILAPI ILboolean ILAPIENTRY ilSavePal(ILconst_string FileName);

ILAPI ILboolean ILAPIENTRY ilSetRead(fOpenProc, fCloseProc, fEofProc, fGetcProc, fReadProc, fSeekProc, fTellProc);
ILAPI ILboolean ILAPIENTRY ilSetWrite(fOpenProc, fCloseProc, fPutcProc, fSeekProc, fTellProc, fWriteProc);

ILAPI ILenum    ILAPIENTRY ilTypeFromExt(ILconst_string FileName);

ILAPI ILboolean ILAPIENTRY ilLoadData(ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILAPI ILboolean ILAPIENTRY ilLoadDataF(ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILAPI ILboolean ILAPIENTRY ilLoadDataL(void *Lump, ILuint Size, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);
ILAPI ILboolean ILAPIENTRY ilSaveData(ILconst_string FileName);


#ifdef IL_VERSION_1_9_0
ILAPI ILboolean ILAPIENTRY ilAddAlpha(void);
ILAPI ILint     ILAPIENTRY ilGetIntegerImage(ILuint Image, ILenum Mode);
#endif

#ifdef IL_VERSION_1_10_0
ILAPI ILboolean ILAPIENTRY ilEnumMetadata(ILuint Index, ILenum *IFD, ILenum *ID);
ILAPI ILboolean ILAPIENTRY ilGetMetadata(ILenum IFD, ILenum ID, ILenum *Type, ILuint *Count, ILuint *Size, void **Data);
ILAPI ILboolean ILAPIENTRY ilSetMetadata(ILenum IFD, ILenum ID, ILenum Type, ILuint Count, ILuint Size, const void *Data);
ILAPI ILfloat   ILAPIENTRY ilGetFloat(ILenum Mode);
ILAPI ILfloat   ILAPIENTRY ilGetFloatImage(ILuint Image, ILenum Mode);
ILAPI ILuint    ILAPIENTRY ilGetFloatv(ILenum Mode, ILfloat *Param);
ILAPI ILuint    ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param);
ILAPI void      ILAPIENTRY ilClearIndex(ILuint index);
ILAPI void      ILAPIENTRY ilClearMetadata(void);
ILAPI void      ILAPIENTRY ilSetFloat(ILenum Mode, ILfloat Param);
ILAPI void      ILAPIENTRY ilSetFloatv(ILenum Mode, ILfloat *Param);
ILAPI void      ILAPIENTRY ilSetIntegerv(ILenum Mode, ILint *Param);
#endif 


// For all those weirdos that spell "colour" without the 'u'.
#define ilClearColor  ilClearColour
#define ilKeyColor    ilKeyColour

#ifdef __cplusplus
}
#endif

#endif // __IL_H__
#endif // __il_h__
