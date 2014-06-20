CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )

PROJECT                 ( IL C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( SOURCES
                          # altivec/common.c
                          # altivec/typeconversion.c

                          algo/il_neuquant.c
                          algo/il_nvidia.cc
                          algo/il_quantizer.c
                          algo/il_rle.c
                          algo/il_squish.c

                          conv/il_convbuff.c
                          conv/il_convert.c 
                          conv/il_fastconv.c

                          formats/il_blp.c
                          formats/il_bmp.c
                          formats/il_cut.c
                          formats/il_dcx.c
                          formats/il_dds-save.c
                          formats/il_dds.c
                          formats/il_dicom.c
                          formats/il_doom.c
                          formats/il_dpx.c
                          formats/il_exr.cc
                          formats/il_fits.c
                          formats/il_ftx.c
                          formats/il_gif.c
                          formats/il_hdr.c
                          formats/il_header.c
                          formats/il_icns.c
                          formats/il_icon.c
                          formats/il_iff.c
                          formats/il_ilbm.c
                          formats/il_iwi.c
                          formats/il_jp2.c
                          formats/il_jpeg.c
                          formats/il_lif.c
                          formats/il_mdl.c
                          formats/il_mng.c
                          formats/il_mp3.c
                          formats/il_pal_jasc.c
                          formats/il_pal_halo.c
                          formats/il_pal_act.c
                          formats/il_pal_col.c
                          formats/il_pal_plt.c
                          formats/il_pcd.c
                          formats/il_pcx.c
                          formats/il_pic.c
                          formats/il_pix.c
                          formats/il_png.c
                          formats/il_pnm.c
                          formats/il_psd.c
                          formats/il_psp.c
                          formats/il_pxr.c
                          formats/il_raw.c
                          formats/il_rawdata.c
                          formats/il_rot.c
                          formats/il_sgi.c
                          formats/il_sun.c
                          formats/il_targa.c
                          formats/il_texture.c
                          formats/il_tiff.c
                          formats/il_tpl.c
                          formats/il_utx.c
                          formats/il_vtf.c
                          formats/il_wal.c 
                          formats/il_wbmp.c
                          formats/il_wdp.c
                          formats/il_xpm.c

                          il_alloc.c
                          il_api.c
                          il_bits.c
                          il_endian.c
                          il_error.c 
                          il_files.c
                          il_formats.c
                          il_internal.c
                          il_io.c
                          il_kail.c
                          il_main.c
                          il_manip.c
                          il_pal.c
                          il_profiles.c
                          il_register.c
                          il_size.c
                          il_skia.cc
                          il_stack.c
                          il_states.c
                          il_string.c
                          il_utility.c
                        )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )
INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR}/algo )
INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR}/conv )
INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR}/formats )

IF                      ( IL_STATIC_LIB )
  SET                   ( SOURCES STATIC ${SOURCES} )
ELSE                    ( )
  SET                   ( SOURCES SHARED ${SOURCES} )
ENDIF                   ( )

SET                     ( LINK_LIBS 
                          ${LINK_LIBS}
                          ${ZLIB_LIBRARIES}
                          ${MNG_LIBRARIES}
                          ${JPEG_LIBRARIES}
                          ${PNG_LIBRARIES}
                          ${TIFF_LIBRARIES}
                          ${ALLEGRO_LIBRARIES}
                          ${OPENGL_LIBRARIES}
                          ${SDL_LIBRARIES}
                          ${X11_LIBRARIES}
                          ${THREADS_LIB}
                        )

IF                      ( NOT IL_NO_JP2 )
  SET                   ( LINK_LIBS ${LINK_LIBS} ${JASPER_LIBRARIES} )
ENDIF                   ()

IF                      ( NOT IL_NO_LCMS )
  SET                   ( LINK_LIBS ${LINK_LIBS} ${LCMS2_LIBRARIES} )
ENDIF                   ()

IF                      ( NOT IL_NO_EXR )
  SET                   ( LINK_LIBS ${LINK_LIBS} ${OPENEXR_LIBRARIES} )
  INCLUDE_DIRECTORIES   ( ${OPENEXR_INCLUDE_DIR} )
ENDIF                   ()

INCLUDE_DIRECTORIES     ( ${Threads_INCLUDE_DIR} )

IF                      ( WIN32 )
  # Windows programs need a DevIL.dll not libIL.dll
  SET                   ( LIBNAME DevIL )
  ADD_LIBRARY           ( ${LIBNAME} ${SOURCES} )
  SET_TARGET_PROPERTIES ( ${LIBNAME} PROPERTIES PREFIX "" )
ELSE                    ()
  SET                   ( LIBNAME IL )
  ADD_LIBRARY           ( ${LIBNAME} ${SOURCES} )
ENDIF                   ()

TARGET_LINK_LIBRARIES   ( ${LIBNAME} ${LINK_LIBS} )

INSTALL                 ( TARGETS ${LIBNAME}
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )
