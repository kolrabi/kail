IF                      ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
  MESSAGE               ( FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there" )
ENDIF                   ( )

CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( KaIL C CXX )

INCLUDE                 ( CTest )

OPTION                  ( TEST_COVERAGE          "Test code coverage" FALSE )

IF                      ( CMAKE_COMPILER_IS_GNUCC )
  SET                   ( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=gnu99 -fopenmp" )
  SET                   ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fopenmp" )

  IF                      ( TEST_COVERAGE )
    MESSAGE               ( "Enabling code coverage testing" )
    SET                   ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage" )
    SET                   ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage" )
    SET                   ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage ")
    SET                   ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage " )
  ENDIF                   (  )

  IF                    ( MINGW )
    SET                   ( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc -static-libstdc++" )
    SET                   ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++" )
  ENDIF                 ()
  SET                   ( RESTRICT_KEYWORD TRUE )
ELSEIF                  ( MSVC )
  SET                   ( CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR}/msvclibs )
ENDIF                   ()

SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

INCLUDE                 ( CheckIncludeFile )
INCLUDE                 ( CheckTypeSize )
INCLUDE                 ( CheckLibraryExists )
INCLUDE                 ( CheckFunctionExists )
INCLUDE                 ( FindPkgConfig )
INCLUDE                 ( TestBigEndian )
INCLUDE                 ( CheckCCompilerFlag )
INCLUDE                 ( TestBigEndian )

IF                      ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  SET                   ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Weverything -Wextra -Wno-documentation-unknown-command -Werror" )
  IF ( NOT WIN32 )
    SET                   ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined,address -fno-omit-frame-pointer" )
  ENDIF ()
  SET                   ( RESTRICT_KEYWORD TRUE )
  IF                      ( TEST_COVERAGE )
    MESSAGE               ( "Enabling code coverage testing" )

    FIND_LIBRARY          ( PROFILE_RT_LIB NAMES libprofile_rt.a )
    MESSAGE ("Profile lib: ${PROFILE_RT_LIB}")
    SET                   ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage" )
    SET                   ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage" )
    SET                   ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage ")
    SET                   ( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage " )
  ENDIF                   ( )
ENDIF()

# HACK: prefer static 
IF                      ( MINGW )
  FIND_LIBRARY          ( PTHREAD_STATIC_LIB NAMES libpthread.a libpthreads.a )
  IF                    ( PTHREAD_STATIC_LIB )

    SET                 ( THREADS_HAVE_PTHREAD_ARG TRUE ) 
    SET                 ( CMAKE_THREAD_LIBS_INIT ${PTHREAD_STATIC_LIB} )
    SET                 ( CMAKE_HAVE_THREADS_LIBRARY TRUE )
    SET                 ( Threads_FOUND TRUE )

    SET                 ( THREADS_LIB ${PTHREAD_STATIC_LIB})
  ENDIF                 ()
ENDIF                   ()

FIND_PACKAGE            ( Threads )

SET                     ( HAVE_PTHREAD      ${CMAKE_USE_PTHREADS_INIT} )
IF                      ( NOT PTHREAD_STATIC_LIB )
    SET                 ( THREADS_LIB ${CMAKE_THREAD_LIBS_INIT})
ENDIF                   ()

IF                      ( HAVE_PTHREAD OR WIN32 )
SET                     ( HAVE_THREADS      TRUE )
ELSE                    ()
SET                     ( HAVE_THREADS      FALSE )
ENDIF                   ()

FIND_PACKAGE            ( OpenGL ) 
FIND_PACKAGE            ( D3D9 )
FIND_PACKAGE            ( D3D8 )
FIND_PACKAGE            ( SDL )
FIND_PACKAGE            ( X11 )
FIND_PACKAGE            ( Squish )
SET                     ( IL_USE_DXTC_SQUISH ${LIBSQUISH_FOUND} )

IF                      ( MINGW )
  FIND_LIBRARY          ( ZLIB_LIBRARY NAMES libz.a ) # HACK for mingw
ENDIF                   ( )
FIND_PACKAGE            ( ZLIB )

CHECK_INCLUDE_FILE      ( "stdint.h"        HAVE_STDINT_H )
CHECK_INCLUDE_FILE      ( "stdbool.h"       HAVE_STDBOOL_H )
CHECK_INCLUDE_FILE      ( "stdlib.h"        HAVE_STDLIB_H )
CHECK_INCLUDE_FILE      ( "string.h"        HAVE_STRING_H )
CHECK_INCLUDE_FILE      ( "strings.h"       HAVE_STRINGS_H )

CHECK_INCLUDE_FILE      ( "sys/stat.h"      HAVE_SYS_STAT_H )
CHECK_INCLUDE_FILE      ( "sys/type.h"      HAVE_SYS_TYPES_H )
CHECK_INCLUDE_FILE      ( "unistd.h"        HAVE_UNISTD_H )

CHECK_INCLUDE_FILE      ( "windows.h"       HAVE_WINDOWS_H )

CHECK_FUNCTION_EXISTS   ( memalign          MEMALIGN )
CHECK_FUNCTION_EXISTS   ( _mm_malloc        MM_MALLOC )
CHECK_FUNCTION_EXISTS   ( posix_memalign    POSIX_MEMALIGN )
CHECK_FUNCTION_EXISTS   ( valloc            VALLOC )

CHECK_C_COMPILER_FLAG   ( "-msse" SSE )
CHECK_C_COMPILER_FLAG   ( "-msse2" SSE2 )
CHECK_C_COMPILER_FLAG   ( "-msse3" SSE3 )

TEST_BIG_ENDIAN         ( WORDS_BIGENDIAN )

SET                     ( HAVE_LIBM         TRUE )

SET                     ( DEBUG             ${CMAKE_BUILD_TYPE} == Debug )

FIND_PACKAGE            ( Allegro )

OPTION                  ( IL_STATIC_LIB          "Build static libraries"         FALSE )
OPTION                  ( ILU_ENABLED            "Build ILU"                      TRUE )
OPTION                  ( IL_THREAD_SAFE         "Build thread safe libraries"    ${HAVE_THREADS} )
OPTION                  ( IL_UNICODE             "Enable unicode"                 FALSE )

OPTION                  ( ILUT_USE_ALLEGRO       "Enable support for Allegro"     ${ALLEGRO_FOUND} )
OPTION                  ( ILUT_USE_DIRECTX8      "Enable support for DirectX 8"   ${D3D8_FOUND} )
OPTION                  ( ILUT_USE_DIRECTX9      "Enable support for DirectX 9"   ${D3D9_FOUND} )
OPTION                  ( ILUT_USE_OPENGL        "Enable support for OpenGL"      ${OPENGL_FOUND} )
OPTION                  ( ILUT_USE_SDL           "Enable support for SDL"         ${SDL_FOUND} )
OPTION                  ( ILUT_USE_X11           "Enable support for X11"         ${X11_FOUND} )
OPTION                  ( ILUT_USE_XRENDER       "Enable support for Xrender"     ${X11_Xrender_FOUND} )
OPTION                  ( ILUT_USE_XSHM          "Enable support for Xshm"        ${X11_XShm_FOUND} )

OPTION                  ( IL_NO_GAMES            "Disable support for all game formats" FALSE )
OPTION                  ( IL_NO_EXTLIBS          "Disable support for all formats that require external libraries" FALSE )

OPTION                  ( IL_NO_BLP              "Disable support for BLP format" FALSE )
OPTION                  ( IL_NO_BMP              "Disable support for BMP format" FALSE )
OPTION                  ( IL_NO_DCX              "Disable support for DCX format" FALSE )
OPTION                  ( IL_NO_DDS              "Disable support for DDS format" FALSE )
OPTION                  ( IL_NO_DICOM            "Disable support for DICOM format" FALSE )
OPTION                  ( IL_NO_DOOM             "Disable support for Doom format" FALSE )
OPTION                  ( IL_NO_EXR              "Disable support for EXR format"  FALSE)
OPTION                  ( IL_NO_FITS             "Disable support for FITS format" FALSE )
OPTION                  ( IL_NO_GIF              "Disable support for GIF format" FALSE )
OPTION                  ( IL_NO_HDR              "Disable support for HDR format" FALSE )
OPTION                  ( IL_NO_ICNS             "Disable support for ICNS format" FALSE )
OPTION                  ( IL_NO_IFF              "Disable support for IFF format" FALSE )
OPTION                  ( IL_NO_ILBM             "Disable support for ILBM format" FALSE )
OPTION                  ( IL_NO_IWI              "Disable support for IWI format" FALSE )
OPTION                  ( IL_NO_JP2              "Disable support for JP2 format" FALSE ) 
OPTION                  ( IL_NO_JPG              "Disable support for JPG format" FALSE )
OPTION                  ( IL_NO_LIF              "Disable support for LIF format" FALSE )
OPTION                  ( IL_NO_MDL              "Disable support for MDL format" FALSE ) 
OPTION                  ( IL_NO_MNG              "Disable support for MNG format" FALSE )
OPTION                  ( IL_NO_MP               "Disable support for MP format" FALSE )
OPTION                  ( IL_NO_PCD              "Disable support for PCD format" FALSE )
OPTION                  ( IL_NO_PCX              "Disable support for PCX format" FALSE )
OPTION                  ( IL_NO_PIC              "Disable support for PIC format" FALSE )
OPTION                  ( IL_NO_PIX              "Disable support for PIX format" FALSE )
OPTION                  ( IL_NO_PNG              "Disable support for PNG format" FALSE )
OPTION                  ( IL_NO_PNM              "Disable support for PNM format" FALSE )
OPTION                  ( IL_NO_PSD              "Disable support for PSD format" FALSE )
OPTION                  ( IL_NO_PSP              "Disable support for PSP format" FALSE )
OPTION                  ( IL_NO_PXR              "Disable support for PXR format" FALSE )
OPTION                  ( IL_NO_RAW              "Disable support for RAW format" FALSE )
OPTION                  ( IL_NO_ROT              "Disable support for ROT format" FALSE )
OPTION                  ( IL_NO_SGI              "Disable support for SGI format" FALSE )
OPTION                  ( IL_NO_SUN              "Disable support for SUN format" FALSE )
OPTION                  ( IL_NO_TEXTURE          "Disable support for TEXTURE format" FALSE )
OPTION                  ( IL_NO_TGA              "Disable support for TGA format" FALSE )
OPTION                  ( IL_NO_TIF              "Disable support for TIF format" FALSE )
OPTION                  ( IL_NO_TPL              "Disable support for TPL format" FALSE )
OPTION                  ( IL_NO_UTX              "Disable support for UTX format" FALSE )
OPTION                  ( IL_NO_VTF              "Disable support for VTF format" FALSE )
OPTION                  ( IL_NO_WAD              "Disable support for WAD format" FALSE )
OPTION                  ( IL_NO_WAL              "Disable support for WAL format" FALSE )
OPTION                  ( IL_NO_WBMP             "Disable support for WBMP format" FALSE )
OPTION                  ( IL_NO_WDP              "Disable support for WDP format" TRUE)
OPTION                  ( IL_NO_XPM              "Disable support for XPM format" FALSE )

SET                     ( IL_NO_SKIA             TRUE )

SET                     ( ILU_STATIC ${IL_STATIC_LIB} )
SET                     ( ILUT_STATIC ${IL_STATIC_LIB} )

IF                      ( NOT IL_NO_JPG )
  IF                    ( MINGW )
    FIND_LIBRARY        ( JPEG_LIBRARY NAMES ${JPEG_NAMES} libjpeg.a ) # HACK for mingw
  ENDIF                 ( )
  FIND_PACKAGE          ( JPEG )
  IF                    ( NOT JPEG_FOUND )
    SET                 ( IL_NO_JPG TRUE )
    MESSAGE             ( WARNING "No jpeglib found. Disabling JPEG support." )
  ENDIF                 ()
ENDIF                   ()

IF                      ( NOT IL_NO_JP2 )
  FIND_PACKAGE          ( Jasper )
  IF                    ( NOT JASPER_FOUND )
    SET                 ( IL_NO_JP2 TRUE )
    MESSAGE             ( WARNING "No Jasper found. Disabling JPEG2000 support." )
  ENDIF                 ()
ENDIF                   ()

IF                      ( NOT IL_NO_MNG )
  FIND_LIBRARY          ( MNG_LIBRARY NAMES libmng.a mng.a mng.lib )
  FIND_PACKAGE          ( MNG )
  IF                    ( NOT MNG_FOUND )
    SET                 ( IL_NO_MNG TRUE )
    MESSAGE             ( WARNING "No libmng found. Disabling MNG support." )
  ENDIF                 ()
ENDIF                   ()

IF                      ( NOT IL_NO_PNG )
  FIND_PACKAGE          ( PNG )
  IF                    ( NOT PNG_FOUND )
    SET                 ( IL_NO_PNG TRUE )
    MESSAGE             ( WARNING "No libpng found. Disabling PNG support." )
  ENDIF                 ()
ENDIF                   ()

IF                      ( NOT IL_NO_TIF )
  FIND_PACKAGE          ( TIFF )
  IF                    ( NOT TIFF_FOUND )
    SET                 ( IL_NO_TIF TRUE )
    MESSAGE             ( WARNING "No libtiff found. Disabling TIFF support." )
  ENDIF                 ()
ENDIF                   ()

IF                      ( NOT IL_NO_EXR )
  FIND_LIBRARY          ( OPENEXR_LIBRARY NAMES IlmImf.a IlmImf libIlmImf IlmImf-2_1 libIlmImf-2_1 )
  FIND_LIBRARY          ( HALF_LIBRARY NAMES Half.a Half.lib Half libHalf)
  FIND_LIBRARY          ( IEX_LIBRARY NAMES Iex.a Iex libIex )
  FIND_LIBRARY          ( ILMTHREAD_LIBRARY NAMES IlmThread libIlmThread )
  #FIND_LIBRARY          ( IMATH_LIBRARY NAMES Imath libImath )
  FIND_PATH             ( OPENEXR_INCLUDE_DIR ImfIO.h PATH_SUFFIXES OpenEXR)
  IF                    ( NOT OPENEXR_LIBRARY       OR 
                          NOT HALF_LIBRARY          OR 
                          NOT OPENEXR_INCLUDE_DIR   OR 
                          NOT IEX_LIBRARY           OR 
                          NOT ILMTHREAD_LIBRARY )
    SET                 ( IL_NO_EXR TRUE )
    MESSAGE             ( WARNING "No libIlmImf found. Disabling EXR support." )
  ELSE                  ()
  MESSAGE(WARNING OpenEXR: ${OPENEXR_INCLUDE_DIR})
    SET                 ( OPENEXR_LIBRARIES 
                          ${OPENEXR_LIBRARY} 
                          ${HALF_LIBRARY} 
                          ${IEX_LIBRARY} 
                          ${ILMTHREAD_LIBRARY} )
  ENDIF                 ()
ENDIF                   ()

IF                      ( NOT IL_NO_LCMS )
  FIND_PACKAGE          ( LCMS2 )
  IF                    ( NOT LCMS2_FOUND )
    SET                 ( IL_NO_LCMS TRUE )
    MESSAGE             ( WARNING "No LCMS2 found. Disabling LCMS support." )
  ELSE                  ()
    IF                  ( NOT ${LCMS2_INCLUDE_DIR} MATCHES "lcms/" )
      SET                 ( LCMS_NODIRINCLUDE TRUE )
    ENDIF               ()
  ENDIF                 ()
ENDIF                   ()


CONFIGURE_FILE          ( ${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/IL/config.h )
CONFIGURE_FILE          ( ${CMAKE_SOURCE_DIR}/include/ilut_config.h.in ${CMAKE_BINARY_DIR}/IL/ilut_config.h )
ADD_DEFINITIONS         ( -DHAVE_CONFIG_H ) 

INCLUDE_DIRECTORIES     ( ${CMAKE_BINARY_DIR} )
INCLUDE_DIRECTORIES     ( ${CMAKE_SOURCE_DIR}/include )

ADD_SUBDIRECTORY        ( src/IL )
ADD_SUBDIRECTORY        ( src/ILU )
ADD_SUBDIRECTORY        ( src/ILUT )
ADD_SUBDIRECTORY        ( include )
ADD_SUBDIRECTORY        ( src/test )

INSTALL                 ( FILES       ${CMAKE_BINARY_DIR}/IL/config.h
                                      ${CMAKE_BINARY_DIR}/IL/ilut_config.h
                          DESTINATION include/IL
                        )
