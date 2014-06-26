CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILU C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( SOURCES

                          ilu_api.c
                          ilu_error.c
                          ilu_filter.c
                          ilu_filter_rcg.c
                          ilu_internal.c
                          ilu_main.c
                          ilu_manip.c
                          ilu_mipmap.c
                          ilu_noise.c
                          ilu_region.c
                          ilu_rotate.c
                          ilu_scale.c
                          ilu_scale2d.c
                          ilu_scale3d.c
                          ilu_scaling.c
                          ilu_states.c
                        )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )

IF                      ( ILU_STATIC )
  SET                   ( SOURCES STATIC ${SOURCES} )
ELSE                    ( )
  SET                   ( SOURCES SHARED ${SOURCES} )
ENDIF                   ( )

ADD_LIBRARY             ( ILU ${SOURCES} )

IF                      ( WIN32 )
  # Windows programs need a ILU.dll not libILU.dll
  SET_TARGET_PROPERTIES ( ILU PROPERTIES PREFIX "" )
  TARGET_LINK_LIBRARIES ( ILU DevIL )
ELSE                    ()
  TARGET_LINK_LIBRARIES ( ILU IL )
ENDIF                   ()

IF                      ( MINGW )
  SET_TARGET_PROPERTIES ( ILU PROPERTIES LINK_FLAGS
                          "-Wl,--output-def,${CMAKE_BINARY_DIR}/ILU.def")
  INSTALL               ( FILES ${CMAKE_BINARY_DIR}/ILU.def DESTINATION lib)  
ENDIF                   ()

INSTALL                 ( TARGETS ILU
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )