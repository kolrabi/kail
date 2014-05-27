CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILtest C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )
INCLUDE_DIRECTORIES     ( ${CMAKE_SOURCE_DIR}/include )
INCLUDE_DIRECTORIES     ( ${CMAKE_SOURCE_DIR}/src )

IF                      ( WIN32 )
  SET                   ( ILLIB         ${CMAKE_BINARY_DIR}/src/IL/libDevIL.dll.a )
ELSE                    ()
  SET                   ( ILLIB         ${CMAKE_BINARY_DIR}/src/IL/libIL.so )
ENDIF                   ()

ADD_EXECUTABLE          ( ILtestMemory      iltest-memory.c )
TARGET_LINK_LIBRARIES   ( ILtestMemory      ${ILLIB} )

ADD_EXECUTABLE          ( ILtestIO          iltest-io.c )
TARGET_LINK_LIBRARIES   ( ILtestIO          ${ILLIB} )

ADD_EXECUTABLE          ( ILtestFormatLoad  iltest-format-load.c )
TARGET_LINK_LIBRARIES   ( ILtestFormatLoad  ${ILLIB} )

ADD_TEST                ( MemoryDefault     ILtestMemory      default_alloc )
ADD_TEST                ( MemoryCustom      ILtestMemory      custom_alloc  )
ADD_TEST                ( MemoryMixed       ILtestMemory      mixed_alloc   )
ADD_TEST                ( MemoryEndian      ILtestMemory      endian        )

ADD_TEST                ( IOOpenFile        ILtestIO          open_read ${CMAKE_CURRENT_SOURCE_DIR}/TestFile.txt )
# ADD_TEST                ( IOOpenLump        ILtestIO          open_read_lump ABCDefghijk )

ADD_TEST                ( FormatLoadBLP     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.blp 600 400 1 0 )
ADD_TEST                ( FormatLoadBMP     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.bmp 600 400 1 0 )
ADD_TEST                ( FormatLoadCUT     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.cut 600 400 1 0 )
ADD_TEST                ( FormatLoadDCX     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.dcx 600 400 1 0 )
ADD_TEST                ( FormatLoadDDS     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.dds 600 400 1 0 )
ADD_TEST                ( FormatLoadDICOM   ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.dicom 600 400 1 0 )
# DOOM
ADD_TEST                ( FormatLoadDPX     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.dpx 600 400 1 0 )
ADD_TEST                ( FormatLoadEXR     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.exr 600 400 1 0 )
ADD_TEST                ( FormatLoadFITS    ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.fits 600 400 3 0 )
ADD_TEST                ( FormatLoadFTX     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.ftx 600 400 1 0 )
ADD_TEST                ( FormatLoadGIF     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.gif 600 400 1 0 )
ADD_TEST                ( FormatLoadGIFAnim ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/animated.gif 256 256 1 99 )
ADD_TEST                ( FormatLoadHDR     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.hdr 600 400 1 0 )
# CHEAD
ADD_TEST                ( FormatLoadICNS    ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.icns 600 400 1 0 )
ADD_TEST                ( FormatLoadICON    ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.ico 600 400 1 0 )
ADD_TEST                ( FormatLoadIFF     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.iff 600 400 1 0 )
ADD_TEST                ( FormatLoadILBM    ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.ilbm 600 400 1 0 )
ADD_TEST                ( FormatLoadIWI     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.iwi 600 400 1 0 )
ADD_TEST                ( FormatLoadJP2     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.jp2 600 400 1 0 )
ADD_TEST                ( FormatLoadJPG     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.jpg 600 400 1 0 )
ADD_TEST                ( FormatLoadLIF     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.lif 600 400 1 0 )
# MDL
ADD_TEST                ( FormatLoadMNG     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.mng 600 400 1 0 )
# MP3
ADD_TEST                ( FormatLoadPCD     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.pcd 600 400 1 0 )
ADD_TEST                ( FormatLoadPCX     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.pcx 600 400 1 0 )
ADD_TEST                ( FormatLoadPIC     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.pic 600 400 1 0 )
ADD_TEST                ( FormatLoadPIX     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.pix 600 400 1 0 )
ADD_TEST                ( FormatLoadPNG     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.png 600 400 1 0 )
ADD_TEST                ( FormatLoadPNM     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.pnm 600 400 1 0 )
ADD_TEST                ( FormatLoadPSD     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.psd 600 400 1 0 )
ADD_TEST                ( FormatLoadPSP     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.psp 600 400 1 0 )
ADD_TEST                ( FormatLoadPXR     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.pxr 600 400 1 0 )
# RAW
ADD_TEST                ( FormatLoadROT     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.rot 600 400 1 0 )
ADD_TEST                ( FormatLoadSGI     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.sgi 600 400 1 0 )
ADD_TEST                ( FormatLoadSUN     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.im24 600 400 1 0 )
ADD_TEST                ( FormatLoadTGA     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.tga 600 400 1 0 )
# TEXTURE
ADD_TEST                ( FormatLoadTIFF    ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.tif 600 400 1 0 )
# TPL
# UTX
# VTF
# WAL
ADD_TEST                ( FormatLoadWBMP    ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.wbmp 600 400 1 0 )
# WDP
ADD_TEST                ( FormatLoadXPM     ILtestFormatLoad  ${CMAKE_SOURCE_DIR}/images/dougie.xpm 600 400 1 0 )
