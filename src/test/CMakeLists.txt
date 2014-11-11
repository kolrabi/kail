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

ADD_TEST                ( MemoryDefault     ILtestMemory      default_alloc )
ADD_TEST                ( MemoryCustom      ILtestMemory      custom_alloc  )
ADD_TEST                ( MemoryMixed       ILtestMemory      mixed_alloc   )
ADD_TEST                ( MemoryEndian      ILtestMemory      endian        )

ADD_TEST                ( ILUErrorStrings   ILtestILUError                  )

ADD_TEST                ( IOOpenFile        ILtestIO          open_read ${CMAKE_CURRENT_SOURCE_DIR}/TestFile.txt )
ADD_TEST                ( IOOpenLump        ILtestIO          open_read_lump ABCDefghijk )

ADD_TEST                ( Convert           ILtestConvert     ${REF_CLR} )

SET                     ( ILU_IMAGE ${CMAKE_SOURCE_DIR}/images/dougie.png )
ADD_TEST                ( ILUAlienify       ILtestILU         ${ILU_IMAGE} ilu_alienify.png alienify )
ADD_TEST                ( ILUBlurAvg        ILtestILU         ${ILU_IMAGE} ilu_bluravg.png  bluravg )
ADD_TEST                ( ILUBlurGaussian   ILtestILU         ${ILU_IMAGE} ilu_blurgaussian.png  blurgaussian )
# iluBuidMipmaps
# iluColoursUsed
# iluCompareImage
ADD_TEST                ( ILUContrast       ILtestILU         ${ILU_IMAGE} ilu_constrast.png     contrast )
# iluConvolution implicitly tested by blurs 
ADD_TEST                ( ILUCrop           ILtestILU         ${ILU_IMAGE} ilu_crop.png          crop )
ADD_TEST                ( ILUEdgeDetectE    ILtestILU         ${ILU_IMAGE} ilu_edge_e.png        edge_e )
ADD_TEST                ( ILUEdgeDetectP    ILtestILU         ${ILU_IMAGE} ilu_edge_p.png        edge_p )
ADD_TEST                ( ILUEdgeDetectS    ILtestILU         ${ILU_IMAGE} ilu_edge_s.png        edge_s )
ADD_TEST                ( ILUEnlargeCanvas  ILtestILU         ${ILU_IMAGE} ilu_canvas.png        canvas )
ADD_TEST                ( ILUEqualize       ILtestILU         ${ILU_IMAGE} ilu_equalize.png      equalize )
ADD_TEST                ( ILUFlipImage      ILtestILU         ${ILU_IMAGE} ilu_flip.png          flip )
ADD_TEST                ( ILUGamma          ILtestILU         ${ILU_IMAGE} ilu_gamma.png         gamma )
ADD_TEST                ( ILUMirror         ILtestILU         ${ILU_IMAGE} ilu_mirror.png        mirror )
ADD_TEST                ( ILUNegative       ILtestILU         ${ILU_IMAGE} ilu_negative.png      negative )
ADD_TEST                ( ILUNoisify        ILtestILU         ${ILU_IMAGE} ilu_noisify.png       noisify )
ADD_TEST                ( ILUNormalize      ILtestILU         ${ILU_IMAGE} ilu_normalize.png     normalize )
ADD_TEST                ( ILUPixelize       ILtestILU         ${ILU_IMAGE} ilu_pixelize.png      pixelize )
# region
ADD_TEST                ( ILUReplace        ILtestILU         ${ILU_IMAGE} ilu_replace.png       replace )
ADD_TEST                ( ILURotate         ILtestILU         ${ILU_IMAGE} ilu_rotate.png        rotate )
ADD_TEST                ( ILUSaturate1      ILtestILU         ${ILU_IMAGE} ilu_saturate1.png     saturate1 )
ADD_TEST                ( ILUSaturate4      ILtestILU         ${ILU_IMAGE} ilu_saturate4.png     saturate4 )
ADD_TEST                ( ILUScaleDNearest  ILtestILU         ${ILU_IMAGE} ilu_scale_down_nearest.png    scale_down_nearest )
ADD_TEST                ( ILUScaleDLinear   ILtestILU         ${ILU_IMAGE} ilu_scale_down_linear.png     scale_down_linear )
ADD_TEST                ( ILUScaleDBilinear ILtestILU         ${ILU_IMAGE} ilu_scale_down_bilinear.png   scale_down_bilinear )
ADD_TEST                ( ILUScaleUNearest  ILtestILU         ${ILU_IMAGE} ilu_scale_up_nearest.png    scale_up_nearest )
ADD_TEST                ( ILUScaleULinear   ILtestILU         ${ILU_IMAGE} ilu_scale_up_linear.png     scale_up_linear )
ADD_TEST                ( ILUScaleUBilinear ILtestILU         ${ILU_IMAGE} ilu_scale_up_bilinear.png   scale_up_bilinear )
ADD_TEST                ( ILUScaleAlpha     ILtestILU         ${ILU_IMAGE} ilu_scale_alpha.png   scalealpha )
ADD_TEST                ( ILUScaleColours   ILtestILU         ${ILU_IMAGE} ilu_scale_colours.png   scalecolours )
ADD_TEST                ( ILUSharpen        ILtestILU         ${ILU_IMAGE} ilu_sharpen.png   sharpen )
ADD_TEST                ( ILUSwap           ILtestILU         ${ILU_IMAGE} ilu_swap.png   swap )
ADD_TEST                ( ILUWave           ILtestILU         ${ILU_IMAGE} ilu_wave.png   wave )

SET                     ( REF_CLR    ${CMAKE_SOURCE_DIR}/images/dougie.png )
SET                     ( REF_BW     ${CMAKE_SOURCE_DIR}/images/dougie-bw.png )
SET                     ( REF_BL     ${CMAKE_SOURCE_DIR}/images/dougie-bl.png )
SET                     ( REF_RG     ${CMAKE_SOURCE_DIR}/images/dougie-rg.png )
SET                     ( REF_ANI    ${CMAKE_SOURCE_DIR}/images/animated-ref.png )
SET                     ( FMT_PFX    ${CMAKE_SOURCE_DIR}/images )
SET                     ( TMP_PFX    ${CMAKE_BINARY_DIR} )

ADD_TEST                ( QuantWu           ILtestAlgoQuant   ${REF_CLR} )
ADD_TEST                ( QuantNeu          ILtestAlgoNeuQuant   ${REF_CLR} )
ADD_TEST                ( BMPRLE            ILtestAlgoBMPRLE     ${REF_CLR} )
ADD_TEST                ( SGIRLE            ILtestAlgoSGIRLE     ${REF_CLR} )
ADD_TEST                ( TGARLE            ILtestAlgoTGARLE     ${REF_CLR} )

ADD_TEST                ( DDSNocomp         ILtestAlgoDDS     ${REF_CLR} 0 070B test_nocomp.dds )
ADD_TEST                ( DDSDXT1           ILtestAlgoDDS     ${REF_CLR} 0 0706 test_dxt1.dds )
ADD_TEST                ( DDSDXT2           ILtestAlgoDDS     ${REF_CLR} 0 0707 test_dxt2.dds )
ADD_TEST                ( DDSDXT3           ILtestAlgoDDS     ${REF_CLR} 0 0708 test_dxt3.dds )
ADD_TEST                ( DDSDXT4           ILtestAlgoDDS     ${REF_CLR} 0 0709 test_dxt4.dds )
ADD_TEST                ( DDSDXT5           ILtestAlgoDDS     ${REF_CLR} 0 070A test_dxt5.dds )
ADD_TEST                ( DDS3DC            ILtestAlgoDDS     ${REF_RG} 0 070E test_3dc.dds )
ADD_TEST                ( DDSRXGB           ILtestAlgoDDS     ${REF_CLR} 0 070F test_rxgb.dds )
ADD_TEST                ( DDSATI1N          ILtestAlgoDDS     ${REF_CLR} 0 0710 test_ati1n.dds )
ADD_TEST                ( DDSDXT1A          ILtestAlgoDDS     ${REF_CLR} 0 0711 test_dxt1a.dds )

ADD_TEST                ( DDSSquishDXT1           ILtestAlgoDDS     ${REF_CLR} 1 0706 test_dxt1_squish.dds )
ADD_TEST                ( DDSSquishDXT2           ILtestAlgoDDS     ${REF_CLR} 1 0707 test_dxt2_squish.dds )
ADD_TEST                ( DDSSquishDXT3           ILtestAlgoDDS     ${REF_CLR} 1 0708 test_dxt3_squish.dds )
ADD_TEST                ( DDSSquishDXT4           ILtestAlgoDDS     ${REF_CLR} 1 0709 test_dxt4_squish.dds )
ADD_TEST                ( DDSSquishDXT5           ILtestAlgoDDS     ${REF_CLR} 1 070A test_dxt5_squish.dds )
ADD_TEST                ( DDSSquish3DC            ILtestAlgoDDS     ${REF_RG} 1 070E test_3dc_squish.dds )
ADD_TEST                ( DDSSquishRXGB           ILtestAlgoDDS     ${REF_CLR} 1 070F test_rxgb_squish.dds )
ADD_TEST                ( DDSSquishATI1N          ILtestAlgoDDS     ${REF_CLR} 1 0710 test_ati1n_squish.dds )
ADD_TEST                ( DDSSquishDXT1A          ILtestAlgoDDS     ${REF_CLR} 1 0711 test_dxt1a_squish.dds )

ADD_TEST                ( FormatLoadBLP     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.blp    600 400 1 0 )
ADD_TEST                ( FormatLoadBMP     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.bmp    600 400 1 0 )
ADD_TEST                ( FormatLoadCUT     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.cut    600 400 1 0 )
ADD_TEST                ( FormatLoadDCX     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dcx    600 400 1 0 )

ADD_TEST                ( FormatLoadDDS0    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS1    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS2    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt1.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS3    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt3.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS4    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt5.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS5    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-ati1.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS6    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-ati2.dds    600 400 1 0 )
ADD_TEST                ( FormatLoadDDS7    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie-dxt5-rxgb.dds    600 400 1 0 )

ADD_TEST                ( FormatLoadDICOM   ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dicom  600 400 1 0 )
# DOOM
ADD_TEST                ( FormatLoadDPX     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.dpx    600 400 1 0 )
ADD_TEST                ( FormatLoadEXR     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.exr    600 400 1 0 )
ADD_TEST                ( FormatLoadFITS    ILtestFormatLoad  ${REF_BW}  ${FMT_PFX}/dougie.fits   600 400 3 0 ) # 3 separate monochrome planes
ADD_TEST                ( FormatLoadFTX     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.ftx    600 400 1 0 )
ADD_TEST                ( FormatLoadGIF     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.gif    600 400 1 0 )
ADD_TEST                ( FormatLoadGIFAnim ILtestFormatLoad  ${REF_ANI} ${FMT_PFX}/animated.gif  256 256 1 99 ) # special resolution, 100 frames
ADD_TEST                ( FormatLoadHDR     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.hdr    600 400 1 0 )
# CHEAD
ADD_TEST                ( FormatLoadICNS    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.icns   512 512 1 5 ) # special resolution, 6 "frames"
ADD_TEST                ( FormatLoadICON    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.ico    600 400 1 0 )
ADD_TEST                ( FormatLoadIFF     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.iff    600 400 1 0 )
ADD_TEST                ( FormatLoadILBM    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.lbm    600 400 1 0 )
ADD_TEST                ( FormatLoadIWI     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.iwi    600 400 1 0 )
ADD_TEST                ( FormatLoadJP2     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.jp2    600 400 1 0 )
ADD_TEST                ( FormatLoadJPG     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.jpg    600 400 1 0 )
ADD_TEST                ( FormatLoadLIF     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.lif    600 400 1 0 )
# MDL
ADD_TEST                ( FormatLoadMNG     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.mng    600 400 1 0 )
# MP3
ADD_TEST                ( FormatLoadPCD     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pcd    768 512 1 0 ) # special resolution 
ADD_TEST                ( FormatLoadPCX     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pcx    600 400 1 0 )
ADD_TEST                ( FormatLoadPIC     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pic    600 400 1 0 )
ADD_TEST                ( FormatLoadPIX     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pix    600 400 1 0 )
ADD_TEST                ( FormatLoadPNG     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.png    600 400 1 0 )
ADD_TEST                ( FormatLoadPBM     ILtestFormatLoad  ${REF_BL}  ${FMT_PFX}/dougie.pbm    600 400 1 0 )
ADD_TEST                ( FormatLoadPGM     ILtestFormatLoad  ${REF_BW}  ${FMT_PFX}/dougie.pgm    600 400 1 0 )
ADD_TEST                ( FormatLoadPPM     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.ppm    600 400 1 0 )
ADD_TEST                ( FormatLoadPNM     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pnm    600 400 1 0 )
ADD_TEST                ( FormatLoadPSD     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.psd    600 400 1 0 )
ADD_TEST                ( FormatLoadPSP     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.psp    600 400 1 0 )
ADD_TEST                ( FormatLoadPXR     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.pxr    600 400 1 0 )
# RAW
ADD_TEST                ( FormatLoadROT     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.rot    600 400 1 0 )
ADD_TEST                ( FormatLoadSGI     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.sgi    600 400 1 0 )
ADD_TEST                ( FormatLoadSUN     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.im24   600 400 1 0 )
ADD_TEST                ( FormatLoadTGA     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.tga    600 400 1 0 )
# TEXTURE
ADD_TEST                ( FormatLoadTIFF    ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.tif    600 400 1 0 )
# TPL
# UTX
# VTF
# WAL
ADD_TEST                ( FormatLoadWBMP    ILtestFormatLoad  ${REF_BL}  ${FMT_PFX}/dougie.wbmp   600 400 1 0 )
# WDP TODO
ADD_TEST                ( FormatLoadXPM     ILtestFormatLoad  ${REF_CLR} ${FMT_PFX}/dougie.xpm    600 400 1 0 )



ADD_TEST                ( FormatSaveBLP     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.blp    )
ADD_TEST                ( FormatSaveBMP     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.bmp    )
ADD_TEST                ( FormatSaveCUT     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.cut    )
ADD_TEST                ( FormatSaveDCX     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dcx    )
ADD_TEST                ( FormatSaveDDS     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dds    )
ADD_TEST                ( FormatSaveDICOM   ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dicom  )
# DOOM
ADD_TEST                ( FormatSaveDPX     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.dpx    )
ADD_TEST                ( FormatSaveEXR     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.exr    )
ADD_TEST                ( FormatSaveFITS    ILtestFormatSave  ${REF_BW}  ${TMP_PFX}/dougie.fits   ) # 3 separate monochrome planes
ADD_TEST                ( FormatSaveFTX     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.ftx    )
ADD_TEST                ( FormatSaveGIF     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.gif    )
ADD_TEST                ( FormatSaveGIFAnim ILtestFormatSave  ${REF_ANI} ${TMP_PFX}/animated.gif  ) # special resolution, 100 frames
ADD_TEST                ( FormatSaveHDR     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.hdr    )
# CHEAD
ADD_TEST                ( FormatSaveICNS    ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.icns   ) # special resolution, 6 "frames"
ADD_TEST                ( FormatSaveICON    ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.ico    )
ADD_TEST                ( FormatSaveIFF     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.iff    )
ADD_TEST                ( FormatSaveILBM    ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.lbm    )
ADD_TEST                ( FormatSaveIWI     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.iwi    )
ADD_TEST                ( FormatSaveJP2     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.jp2    )
ADD_TEST                ( FormatSaveJPG     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.jpg    )
ADD_TEST                ( FormatSaveLIF     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.lif    )
# MDL
ADD_TEST                ( FormatSaveMNG     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.mng    )
# MP3
ADD_TEST                ( FormatSavePCD     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pcd    ) # special resolution 
ADD_TEST                ( FormatSavePCX     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pcx    )
ADD_TEST                ( FormatSavePIC     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pic    )
ADD_TEST                ( FormatSavePIX     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pix    )
ADD_TEST                ( FormatSavePNG     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.png    )
ADD_TEST                ( FormatSavePPM     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.ppm    )
ADD_TEST                ( FormatSavePGM     ILtestFormatSave  ${REF_BW}  ${TMP_PFX}/dougie.pgm    )
ADD_TEST                ( FormatSavePBM     ILtestFormatSave  ${REF_BL}  ${TMP_PFX}/dougie.pbm    )
ADD_TEST                ( FormatSavePNM     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pnm    )
ADD_TEST                ( FormatSavePSD     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.psd    )
ADD_TEST                ( FormatSavePSP     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.psp    )
ADD_TEST                ( FormatSavePXR     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.pxr    )
# RAW
ADD_TEST                ( FormatSaveROT     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.rot    )
ADD_TEST                ( FormatSaveSGI     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.sgi    )
ADD_TEST                ( FormatSaveSUN     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.im24   )
ADD_TEST                ( FormatSaveTGA     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.tga    )
# TEXTURE
ADD_TEST                ( FormatSaveTIFF    ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.tif    )
# TPL
# UTX
# VTF
# WAL
ADD_TEST                ( FormatSaveWBMP    ILtestFormatSave  ${REF_BW}  ${TMP_PFX}/dougie.wbmp   )
# WDP TODO
ADD_TEST                ( FormatSaveXPM     ILtestFormatSave  ${REF_CLR} ${TMP_PFX}/dougie.xpm    )
