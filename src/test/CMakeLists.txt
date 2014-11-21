CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILtest C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )
INCLUDE_DIRECTORIES     ( ${CMAKE_SOURCE_DIR}/include )
INCLUDE_DIRECTORIES     ( ${CMAKE_SOURCE_DIR}/src )

IF                      ( WIN32 )
  SET                   ( ILLIB         DevIL )
ELSE                    ()
  SET                   ( ILLIB         IL )
ENDIF                   ()

ADD_EXECUTABLE          ( ILtestFormatLoad  iltest-format-load.c   )
ADD_EXECUTABLE          ( ILtestFormatSave  iltest-format-save.c   )
ADD_EXECUTABLE          ( ILtestILU         iltest-ilu.c           )
ADD_EXECUTABLE          ( ILtestILUError    iltest-ilu_error.c     )
ADD_EXECUTABLE          ( ILtestIO          iltest-io.c            )
ADD_EXECUTABLE          ( ILtestMemory      iltest-memory.c        )
ADD_EXECUTABLE          ( ILtestAlgoQuant   iltest-algo-quant.c    )
ADD_EXECUTABLE          ( ILtestAlgoNeuQuant   iltest-algo-neuquant.c    )
ADD_EXECUTABLE          ( ILtestAlgoBMPRLE  iltest-algo-bmprle.c    )
ADD_EXECUTABLE          ( ILtestAlgoSGIRLE  iltest-algo-sgirle.c    )
ADD_EXECUTABLE          ( ILtestAlgoTGARLE  iltest-algo-tgarle.c    )
ADD_EXECUTABLE          ( ILtestAlgoDDS  iltest-algo-dds.c    )
ADD_EXECUTABLE          ( ILtestConvert  iltest-convert.c    )

TARGET_LINK_LIBRARIES   ( ILtestFormatLoad  ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestFormatSave  ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestILU         ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestILUError    ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestIO          ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestMemory      ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestAlgoQuant   ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestAlgoNeuQuant   ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestAlgoBMPRLE   ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestAlgoSGIRLE   ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestAlgoTGARLE   ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestAlgoDDS   ${ILLIB} ILU )
TARGET_LINK_LIBRARIES   ( ILtestConvert   ${ILLIB} ILU )

# -----------------------------------------------------------------------------
# reference images
# -----------------------------------------------------------------------------

# true color reference
SET                     ( REF_CLR    ${CMAKE_SOURCE_DIR}/images/dougie.png )

# grayscale reference
SET                     ( REF_BW     ${CMAKE_SOURCE_DIR}/images/dougie-bw.png )

# bilevel reference
SET                     ( REF_BL     ${CMAKE_SOURCE_DIR}/images/dougie-bl.png )

# red/green only reference (for 3DC compression)
SET                     ( REF_RG     ${CMAKE_SOURCE_DIR}/images/dougie-rg.png )

# red/green only reference (for rxgb compression)
SET                     ( REF_RXGB   ${CMAKE_SOURCE_DIR}/images/dougie-rxgb.png )

# animated reference
SET                     ( REF_ANI    ${CMAKE_SOURCE_DIR}/images/animated-ref.png )

# image to use by ILU tests
SET                     ( ILU_IMAGE  ${REF_CLR} )

# -----------------------------------------------------------------------------
# test memory functions
# -----------------------------------------------------------------------------

ADD_TEST                ( MemoryDefault             ILtestMemory        default_alloc )
ADD_TEST                ( MemoryCustom              ILtestMemory        custom_alloc  )
ADD_TEST                ( MemoryMixed               ILtestMemory        mixed_alloc   )
ADD_TEST                ( MemoryEndian              ILtestMemory        endian        )

# -----------------------------------------------------------------------------
# test disk io functions
# -----------------------------------------------------------------------------

ADD_TEST                ( IO_File                   ILtestIO            open_read       ${CMAKE_CURRENT_SOURCE_DIR}/TestFile.txt )
ADD_TEST                ( IO_Lump                   ILtestIO            open_read_lump  ABCDefghijk )

# -----------------------------------------------------------------------------
# test quantization algorithms
# -----------------------------------------------------------------------------

ADD_TEST                ( Quant_Wu                  ILtestAlgoQuant     ${REF_CLR} )
ADD_TEST                ( Quant_Neu                 ILtestAlgoNeuQuant  ${REF_CLR} )

# -----------------------------------------------------------------------------
# test compression algorithms
# -----------------------------------------------------------------------------

ADD_TEST                ( RLE_BMP8                  ILtestAlgoBMPRLE  ${REF_CLR} )
ADD_TEST                ( RLE_SGI                   ILtestAlgoSGIRLE  ${REF_CLR} )
ADD_TEST                ( RLE_Targa                 ILtestAlgoTGARLE  ${REF_CLR} )

ADD_TEST                ( DDS_Nocomp                ILtestAlgoDDS     ${REF_CLR}  0 070B test_nocomp.dds )
ADD_TEST                ( DDS_DXT1                  ILtestAlgoDDS     ${REF_CLR}  0 0706 test_dxt1.dds )
ADD_TEST                ( DDS_DXT2                  ILtestAlgoDDS     ${REF_CLR}  0 0707 test_dxt2.dds )
ADD_TEST                ( DDS_DXT3                  ILtestAlgoDDS     ${REF_CLR}  0 0708 test_dxt3.dds )
ADD_TEST                ( DDS_DXT4                  ILtestAlgoDDS     ${REF_CLR}  0 0709 test_dxt4.dds )
ADD_TEST                ( DDS_DXT5                  ILtestAlgoDDS     ${REF_CLR}  0 070A test_dxt5.dds )
ADD_TEST                ( DDS_3DC                   ILtestAlgoDDS     ${REF_RG}   0 070E test_3dc.dds )
ADD_TEST                ( DDS_RXGB                  ILtestAlgoDDS     ${REF_CLR}  0 070F test_rxgb.dds )
ADD_TEST                ( DDS_ATI1N                 ILtestAlgoDDS     ${REF_CLR}  0 0710 test_ati1n.dds )
ADD_TEST                ( DDS_DXT1A                 ILtestAlgoDDS     ${REF_CLR}  0 0711 test_dxt1a.dds )

ADD_TEST                ( DDS_Squish_DXT1           ILtestAlgoDDS     ${REF_CLR}  1 0706 test_dxt1_squish.dds )
ADD_TEST                ( DDS_Squish_DXT2           ILtestAlgoDDS     ${REF_CLR}  1 0707 test_dxt2_squish.dds )
ADD_TEST                ( DDS_Squish_DXT3           ILtestAlgoDDS     ${REF_CLR}  1 0708 test_dxt3_squish.dds )
ADD_TEST                ( DDS_Squish_DXT4           ILtestAlgoDDS     ${REF_CLR}  1 0709 test_dxt4_squish.dds )
ADD_TEST                ( DDS_Squish_DXT5           ILtestAlgoDDS     ${REF_CLR}  1 070A test_dxt5_squish.dds )
ADD_TEST                ( DDS_Squish_3DC            ILtestAlgoDDS     ${REF_RG}   1 070E test_3dc_squish.dds )
ADD_TEST                ( DDS_Squish_RXGB           ILtestAlgoDDS     ${REF_CLR}  1 070F test_rxgb_squish.dds )
ADD_TEST                ( DDS_Squish_ATI1N          ILtestAlgoDDS     ${REF_CLR}  1 0710 test_ati1n_squish.dds )
ADD_TEST                ( DDS_Squish_DXT1A          ILtestAlgoDDS     ${REF_CLR}  1 0711 test_dxt1a_squish.dds )

# -----------------------------------------------------------------------------
# test autodetecting and loading different formats
# -----------------------------------------------------------------------------

# where to find input images
SET                     ( FMT_PFX                   ${CMAKE_SOURCE_DIR}/images )

ADD_TEST                ( Load_BLP                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.blp    600 400 1 0 )
ADD_TEST                ( Load_BMP_32               ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie32.bmp  600 400 1 0 )
ADD_TEST                ( Load_BMP_24               ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie24.bmp  600 400 1 0 )
ADD_TEST                ( Load_BMP_8                ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie8.bmp     600 400 1 0 )
ADD_TEST                ( Load_BMP_8rle             ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie8-rle.bmp 600 400 1 0 )
ADD_TEST                ( Load_BMP_4                ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie4.bmp     600 400 1 0 )
ADD_TEST                ( Load_BMP_1                ILtestFormatLoad  ${REF_BW}  ${FMT_PFX}/dougie1.bmp     600 400 1 0 )
ADD_TEST                ( Load_CUT                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.cut    600 400 1 0 )
ADD_TEST                ( Load_DCX                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dcx    600 400 1 0 )

ADD_TEST                ( Load_DDS1                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dds              600 400 1 0 )
ADD_TEST                ( Load_DDS2                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt1.dds         600 400 1 0 )
ADD_TEST                ( Load_DDS3                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt3.dds         600 400 1 0 )
ADD_TEST                ( Load_DDS4                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt5.dds         600 400 1 0 )
ADD_TEST                ( Load_DDS5                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-ati1.dds         600 400 1 0 )
ADD_TEST                ( Load_DDS6                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-ati2.dds         600 400 1 0 )
ADD_TEST                ( Load_DDS7                 ILtestFormatLoad  ${REF_RXGB} ${FMT_PFX}/dougie-dxt5-rxgb.dds    600 400 1 0 )

ADD_TEST                ( Load_DICOM                ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dicom  600 400 1 0 )
# DOOM has no autodetection
ADD_TEST                ( Load_DPX                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dpx    600 400 1 0 )
ADD_TEST                ( Load_EXR                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.exr    600 400 1 0 )
ADD_TEST                ( Load_FITS                 ILtestFormatLoad  ${REF_BW}  ${FMT_PFX}/dougie.fits   600 400 3 0 ) # 3 separate monochrome planes
ADD_TEST                ( Load_FTX                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.ftx    600 400 1 0 )
ADD_TEST                ( Load_GIF                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.gif    600 400 1 0 )
ADD_TEST                ( Load_GIFAnim              ILtestFormatLoad  ${REF_ANI} ${FMT_PFX}/animated.gif  256 256 1 99 ) # special resolution, 100 frames
ADD_TEST                ( Load_HDR                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.hdr    600 400 1 0 )
# CHEAD has no autodetection
ADD_TEST                ( Load_ICNS                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.icns   512 512 1 5 ) # special resolution, 6 "frames"
ADD_TEST                ( Load_ICON                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.ico    600 400 1 0 )
ADD_TEST                ( Load_IFF                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.iff    600 400 1 0 )
ADD_TEST                ( Load_ILBM                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.lbm    600 400 1 0 )
ADD_TEST                ( Load_IWI                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.iwi    600 400 1 0 )
ADD_TEST                ( Load_JP2                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.jp2    600 400 1 0 )
ADD_TEST                ( Load_JPG                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.jpg    600 400 1 0 )
ADD_TEST                ( Load_LIF                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.lif    600 400 1 0 )
# MDL has no autodetection
ADD_TEST                ( Load_MNG                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.mng    600 400 1 0 )
# MP3 has no autodetection
ADD_TEST                ( Load_PCD                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pcd    768 512 1 0 ) # special resolution 
ADD_TEST                ( Load_PCX                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pcx    600 400 1 0 )
ADD_TEST                ( Load_PIC                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pic    600 400 1 0 )
ADD_TEST                ( Load_PIX                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pix    600 400 1 0 )
ADD_TEST                ( Load_PNG                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.png    600 400 1 0 )
ADD_TEST                ( Load_PBM                  ILtestFormatLoad  ${REF_BW}  ${FMT_PFX}/dougie.pbm    600 400 1 0 )
ADD_TEST                ( Load_PGM                  ILtestFormatLoad  ${REF_BW}  ${FMT_PFX}/dougie.pgm    600 400 1 0 )
ADD_TEST                ( Load_PPM                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.ppm    600 400 1 0 )
ADD_TEST                ( Load_PNM                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pnm    600 400 1 0 )
ADD_TEST                ( Load_PSD                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.psd    600 400 1 0 )
ADD_TEST                ( Load_PSP                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.psp    600 400 1 0 )
ADD_TEST                ( Load_PXR                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pxr    600 400 1 0 )
# RAW has no autodetection
ADD_TEST                ( Load_ROT                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.rot    600 400 1 0 )
ADD_TEST                ( Load_SGI                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.sgi    600 400 1 0 )
ADD_TEST                ( Load_SUN                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.im24   600 400 1 0 )
ADD_TEST                ( Load_TGA                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.tga    600 400 1 0 )
# TEXTURE has no autodetection
ADD_TEST                ( Load_TIFF                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.tif    600 400 1 0 )
# TPL has no autodetection
# UTX has no autodetection
# VTF has no autodetection
# WAL has no autodetection
ADD_TEST                ( Load_WBMP                 ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.wbmp   600 400 1 0 )
# WDP TODO is not yet implemented
ADD_TEST                ( Load_XPM                  ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.xpm    600 400 1 0 )

# -----------------------------------------------------------------------------
# test saving different formats
# -----------------------------------------------------------------------------

# where to save image files
SET                     ( TMP_PFX    ${CMAKE_BINARY_DIR} )

ADD_TEST                ( Save_BLP                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.blp    )
ADD_TEST                ( Save_BMP                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.bmp    )
ADD_TEST                ( Save_CUT_PAL              ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pal    0476  )
ADD_TEST                ( Save_CUT                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.cut    )
ADD_TEST                ( Save_DCX                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dcx    )
ADD_TEST                ( Save_DDS                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dds    )
ADD_TEST                ( Save_DICOM                ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dicom  )
# DOOM
ADD_TEST                ( Save_DPX                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dpx    )
ADD_TEST                ( Save_EXR                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.exr    )
ADD_TEST                ( Save_FITS                 ILtestFormatSave  ${REF_BW}  ${TMP_PFX}/dougie.fits   ) # 3 separate monochrome planes
ADD_TEST                ( Save_FTX                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.ftx    )
ADD_TEST                ( Save_GIF                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.gif    )
ADD_TEST                ( Save_GIFAnim              ILtestFormatSave  ${REF_ANI} ${TMP_PFX}/animated.gif  ) # special resolution, 100 frames
ADD_TEST                ( Save_HDR                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.hdr    )
# CHEAD
ADD_TEST                ( Save_ICNS                 ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.icns   ) # special resolution, 6 "frames"
ADD_TEST                ( Save_ICON                 ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.ico    )
ADD_TEST                ( Save_IFF                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.iff    )
ADD_TEST                ( Save_ILBM                 ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.lbm    )
ADD_TEST                ( Save_IWI                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.iwi    )
ADD_TEST                ( Save_JP2                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.jp2    )
ADD_TEST                ( Save_JPG                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.jpg    )
ADD_TEST                ( Save_LIF                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.lif    )
# MDL
ADD_TEST                ( Save_MNG                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.mng    )
# MP3
ADD_TEST                ( Save_PCD                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pcd    ) # special resolution 
ADD_TEST                ( Save_PCX                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pcx    )
ADD_TEST                ( Save_PIC                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pic    )
ADD_TEST                ( Save_PIX                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pix    )
ADD_TEST                ( Save_PNG                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.png    )
ADD_TEST                ( Save_PPM                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.ppm    )
ADD_TEST                ( Save_PGM                  ILtestFormatSave  ${REF_BW}  ${TMP_PFX}/dougie.pgm    )
ADD_TEST                ( Save_PBM                  ILtestFormatSave  ${REF_BW}  ${TMP_PFX}/dougie.pbm    )
ADD_TEST                ( Save_PNM                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pnm    )
ADD_TEST                ( Save_PSD                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.psd    )
ADD_TEST                ( Save_PSP                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.psp    )
ADD_TEST                ( Save_PXR                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pxr    )
# RAW
ADD_TEST                ( Save_ROT                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.rot    )
ADD_TEST                ( Save_SGI                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.sgi    )
ADD_TEST                ( Save_SUN                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.im24   )
ADD_TEST                ( Save_TGA                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.tga    )
# TEXTURE
ADD_TEST                ( Save_TIFF                 ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.tif    )
# TPL
# UTX
# VTF
# WAL
ADD_TEST                ( Save_WBMP                 ILtestFormatSave  ${REF_BL}  ${TMP_PFX}/dougie.wbmp   )
# WDP TODO
ADD_TEST                ( Save_XPM                  ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.xpm    )

# -----------------------------------------------------------------------------
# test all possible format and type conversions (slow)
# -----------------------------------------------------------------------------

# ADD_TEST                ( Convert            ILtestConvert     ${REF_CLR} )

# -----------------------------------------------------------------------------
# test ILU functionality, mainly just applying filters and writing images
# for visual inspection. (TODO: automate by comparison with reference images)
# -----------------------------------------------------------------------------

ADD_TEST                ( ILU_Alienify              ILtestILU         ${ILU_IMAGE} ilu_alienify.png            alienify )
ADD_TEST                ( ILU_BlurAvg               ILtestILU         ${ILU_IMAGE} ilu_bluravg.png             bluravg )
ADD_TEST                ( ILU_BlurGaussian          ILtestILU         ${ILU_IMAGE} ilu_blurgaussian.png        blurgaussian )
# iluBuidMipmaps
# iluColoursUsed
# iluCompareImage
ADD_TEST                ( ILU_Contrast              ILtestILU         ${ILU_IMAGE} ilu_constrast.png           contrast )
# iluConvolution implicitly tested by blurs 
ADD_TEST                ( ILU_Crop                  ILtestILU         ${ILU_IMAGE} ilu_crop.png                crop )
ADD_TEST                ( ILU_EdgeDetectE           ILtestILU         ${ILU_IMAGE} ilu_edge_e.png              edge_e )
ADD_TEST                ( ILU_EdgeDetectP           ILtestILU         ${ILU_IMAGE} ilu_edge_p.png              edge_p )
ADD_TEST                ( ILU_EdgeDetectS           ILtestILU         ${ILU_IMAGE} ilu_edge_s.png              edge_s )
ADD_TEST                ( ILU_EnlargeCanvas         ILtestILU         ${ILU_IMAGE} ilu_canvas.png              canvas )
ADD_TEST                ( ILU_Equalize              ILtestILU         ${ILU_IMAGE} ilu_equalize.png            equalize )
ADD_TEST                ( ILU_FlipImage             ILtestILU         ${ILU_IMAGE} ilu_flip.png                flip )
ADD_TEST                ( ILU_Gamma                 ILtestILU         ${ILU_IMAGE} ilu_gamma.png               gamma )
ADD_TEST                ( ILU_Mirror                ILtestILU         ${ILU_IMAGE} ilu_mirror.png              mirror )
ADD_TEST                ( ILU_Negative              ILtestILU         ${ILU_IMAGE} ilu_negative.png            negative )
ADD_TEST                ( ILU_Noisify               ILtestILU         ${ILU_IMAGE} ilu_noisify.png             noisify )
ADD_TEST                ( ILU_Normalize             ILtestILU         ${ILU_IMAGE} ilu_normalize.png           normalize )
ADD_TEST                ( ILU_Pixelize              ILtestILU         ${ILU_IMAGE} ilu_pixelize.png            pixelize )
# region
ADD_TEST                ( ILU_Replace               ILtestILU         ${ILU_IMAGE} ilu_replace.png             replace )
ADD_TEST                ( ILU_Rotate                ILtestILU         ${ILU_IMAGE} ilu_rotate.png              rotate )
ADD_TEST                ( ILU_Saturate1             ILtestILU         ${ILU_IMAGE} ilu_saturate1.png           saturate1 )
ADD_TEST                ( ILU_Saturate4             ILtestILU         ${ILU_IMAGE} ilu_saturate4.png           saturate4 )
ADD_TEST                ( ILU_ScaleDNearest         ILtestILU         ${ILU_IMAGE} ilu_scale_down_nearest.png  scale_down_nearest )
ADD_TEST                ( ILU_ScaleDLinear          ILtestILU         ${ILU_IMAGE} ilu_scale_down_linear.png   scale_down_linear )
ADD_TEST                ( ILU_ScaleDBilinear        ILtestILU         ${ILU_IMAGE} ilu_scale_down_bilinear.png scale_down_bilinear )
ADD_TEST                ( ILU_ScaleUNearest         ILtestILU         ${ILU_IMAGE} ilu_scale_up_nearest.png    scale_up_nearest )
ADD_TEST                ( ILU_ScaleULinear          ILtestILU         ${ILU_IMAGE} ilu_scale_up_linear.png     scale_up_linear )
ADD_TEST                ( ILU_ScaleUBilinear        ILtestILU         ${ILU_IMAGE} ilu_scale_up_bilinear.png   scale_up_bilinear )
ADD_TEST                ( ILU_ScaleAlpha            ILtestILU         ${ILU_IMAGE} ilu_scale_alpha.png         scalealpha )
ADD_TEST                ( ILU_ScaleColours          ILtestILU         ${ILU_IMAGE} ilu_scale_colours.png       scalecolours )
ADD_TEST                ( ILU_Sharpen               ILtestILU         ${ILU_IMAGE} ilu_sharpen.png             sharpen )
ADD_TEST                ( ILU_Swap                  ILtestILU         ${ILU_IMAGE} ilu_swap.png                swap )
ADD_TEST                ( ILU_Wave                  ILtestILU         ${ILU_IMAGE} ilu_wave.png                wave )

ADD_TEST                ( ILU_ErrorStrings          ILtestILUError                  )
