CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILUT C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( SOURCES

                          ilut_allegro.cc
                          ilut_directx.c
                          ilut_directx9.c
                          ilut_internal.c
                          ilut_main.c
                          ilut_opengl.c
                          ilut_sdlsurface.c
                          ilut_states.c
                          ilut_win32.c
                          ilut_x11.c
                        )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )

IF                      ( ILUT_STATIC )
ADD_LIBRARY             ( ILUT STATIC ${SOURCES} )
ELSE                    ( )
ADD_LIBRARY             ( ILUT SHARED ${SOURCES} )
ENDIF                   ( )

IF                      ( WIN32 )
  # Windows programs need a ILUT.dll not libILUT.dll
  SET_TARGET_PROPERTIES ( ILUT PROPERTIES PREFIX "" )
  TARGET_LINK_LIBRARIES ( ILUT ${CMAKE_BINARY_DIR}/src/IL/libDevIL.dll.a )
  TARGET_LINK_LIBRARIES ( ILUT ${CMAKE_BINARY_DIR}/src/ILU/libILU.dll.a )
ELSE                    ()
  TARGET_LINK_LIBRARIES ( ILUT ${CMAKE_BINARY_DIR}/src/IL/libIL.so )
  TARGET_LINK_LIBRARIES ( ILUT ${CMAKE_BINARY_DIR}/src/ILU/libILU.so )
ENDIF                   ()

TARGET_LINK_LIBRARIES   ( ILUT 
                          ${OPENGL_LIBRARIES}
                          ${SDL_LIBRARIES}
                          ${X11_LIBRARIES}
                        )

INSTALL                 ( TARGETS ILUT
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )