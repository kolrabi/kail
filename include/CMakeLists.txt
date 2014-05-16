CMAKE_MINIMUM_REQUIRED  ( VERSION 2.6 )
PROJECT                 ( ILIncludes C CXX )
SET                     ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

SET                     ( HEADERS
                          IL/devil_cpp_wrapper.hpp
                          IL/devil_internal_exports.h
                          IL/il.h
                          IL/ilu.h
                          IL/ilu_region.h
                          IL/ilut.h
                        )

INSTALL                 ( FILES       ${HEADERS}
                          DESTINATION include/IL
                        )