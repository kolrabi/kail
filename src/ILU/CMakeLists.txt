CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILU C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( SOURCES

                          ilu_alloc.c
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
                          ilu_utilities.c
                        )

INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR} )
#INCLUDE_DIRECTORIES     ( ${CMAKE_CURRENT_SOURCE_DIR}/formats )

IF                      ( ILU_STATIC )
ADD_LIBRARY             ( ILU STATIC ${SOURCES} )
ELSE                    ( )
ADD_LIBRARY             ( ILU SHARED ${SOURCES} )
ENDIF                   ( )

INSTALL                 ( TARGETS ILU
                          RUNTIME DESTINATION bin
                          LIBRARY DESTINATION lib
                          ARCHIVE DESTINATION lib
                        )