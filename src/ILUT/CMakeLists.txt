CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILUT C )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( SOURCES

                          ilut_internal.c
                          ilut_states.c
                        )

IF                      ( ILUT_USE_ALLEGRO )
  SET                   ( SOURCES ${SOURCES} ilut_allegro.c )
ENDIF                   ( )

IF                      ( ILUT_USE_DIRECTX8 )
  SET                   ( SOURCES ${SOURCES} ilut_directx.c )
ENDIF                   ( )

IF                      ( ILUT_USE_DIRECTX9 )
  SET                   ( SOURCES ${SOURCES} ilut_directx9.c )
ENDIF                   ( )

IF                      ( ILUT_USE_OPENGL )
  SET                   ( SOURCES ${SOURCES} ilut_opengl.c )
ENDIF                   ( )

IF                      ( ILUT_USE_SDL )
  SET                   ( SOURCES ${SOURCES} ilut_sdlsurface.c )
ENDIF                   ( )

IF                      ( ILUT_USE_SDL2 )
  SET                   ( SOURCES ${SOURCES} ilut_sdl2surface.c )
ENDIF                   ( )

IF                      ( ILUT_USE_X11 )
  SET                   ( SOURCES ${SOURCES} ilut_x11.c )
ENDIF                   ( )

IF                      ( WIN32 )
  SET                   ( SOURCES ${SOURCES} ilut_win32.c )
ENDIF                   ( )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )

IF                      ( ILUT_STATIC )
ADD_LIBRARY             ( ILUT STATIC ${SOURCES} )
ELSE                    ( )
ADD_LIBRARY             ( ILUT SHARED ${SOURCES} )
ENDIF                   ( )

IF                      ( WIN32 )
  # Windows programs need a ILUT.dll not libILUT.dll
  SET_TARGET_PROPERTIES ( ILUT PROPERTIES PREFIX "" )
  TARGET_LINK_LIBRARIES ( ILUT DevIL ILU )
ELSE                    ()
  TARGET_LINK_LIBRARIES ( ILUT IL ILU )
ENDIF                   ()

IF                      ( MINGW )
  SET_TARGET_PROPERTIES ( ILUT PROPERTIES LINK_FLAGS
                          "-Wl,--output-def,${CMAKE_BINARY_DIR}/ILUT.def")
  INSTALL               ( FILES ${CMAKE_BINARY_DIR}/ILUT.def DESTINATION lib)  
ENDIF                   ()

TARGET_LINK_LIBRARIES   ( ILUT 
                          ${OPENGL_LIBRARIES}
                          ${SDL_LIBRARIES}
                          ${X11_LIBRARIES}
                        )

IF                      ( ILUT_USE_SDL )
  INCLUDE_DIRECTORIES   ( ${SDL_INCLUDE_DIR} )
  TARGET_LINK_LIBRARIES ( ILUT ${SDL_LIBRARY} )
ENDIF                   ()

IF                      ( ILUT_USE_ALLEGRO )
  TARGET_LINK_LIBRARIES ( ILUT ${ALLEGRO_LIBRARY} )
ENDIF                   ()

CONFIGURE_FILE          ( ${CMAKE_CURRENT_SOURCE_DIR}/ILUT.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ILUT.pc ESCAPE_QUOTES )
INSTALL                 ( FILES ${CMAKE_CURRENT_BINARY_DIR}/ILUT.pc DESTINATION lib/pkgconfig )

INSTALL                 ( TARGETS ILUT
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )