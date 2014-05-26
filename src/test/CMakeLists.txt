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

ADD_TEST                ( MemoryDefault     ILtestMemory      default_alloc )
ADD_TEST                ( MemoryCustom      ILtestMemory      custom_alloc  )
ADD_TEST                ( MemoryMixed       ILtestMemory      mixed_alloc   )
ADD_TEST                ( MemoryEndian      ILtestMemory      endian        )
