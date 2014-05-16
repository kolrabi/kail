CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILUT C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( SOURCES

                          ilut_allegro.cc
                          ilut_directx.cc
                          ilut_directx9.cc
                          ilut_internal.c
                          ilut_main.c
                          ilut_opengl.c
                          ilut_sdlsurface.c
                          ilut_win32.c
                          ilut_x11.c
                        )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )

IF                      ( ILUT_STATIC )
ADD_LIBRARY             ( ILUT STATIC ${SOURCES} )
ELSE                    ( )
ADD_LIBRARY             ( ILUT SHARED ${SOURCES} )
ENDIF                   ( )

INSTALL                 ( TARGETS ILUT
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )