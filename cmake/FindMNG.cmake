# - Find the native MNG includes and library
#
# This module searches libmng, the library for working with MNG images.
#
# It defines the following variables
#  MNG_INCLUDE_DIRS, where to find mng.h, etc.
#  MNG_LIBRARIES, the libraries to link against to use MNG.
#  MNG_DEFINITIONS - You should add_definitons(${MNG_DEFINITIONS}) before compiling code that includes mng library files.
#  MNG_FOUND, If false, do not try to use MNG.
#  MNG_VERSION_STRING - the version of the MNG library found (since CMake 2.8.8)
# Also defined, but not for general use are
#  MNG_LIBRARY, where to find the MNG library.
# For backward compatiblity the variable MNG_INCLUDE_DIR is also set. It has the same value as MNG_INCLUDE_DIRS.
#
# Since MNG depends on the ZLib compression library, none of the above will be
# defined unless ZLib can be found.

#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if(MNG_FIND_QUIETLY)
  set(_FIND_ZLIB_ARG QUIET)
endif()
find_package(ZLIB ${_FIND_ZLIB_ARG})

if(ZLIB_FOUND)
  find_path(MNG_MNG_INCLUDE_DIR libmng.h
  /usr/local/include/libmng             # OpenBSD
  )

  set(MNG_NAMES ${MNG_NAMES} mng libmng mng15 libmng15 mng15d libmng15d mng14 libmng14 mng14d libmng14d mng12 libmng12 mng12d libmng12d)
  find_library(MNG_LIBRARY NAMES ${MNG_NAMES} )

  if (MNG_LIBRARY AND MNG_MNG_INCLUDE_DIR)
      # mng.h includes zlib.h. Sigh.
      set(MNG_INCLUDE_DIRS ${MNG_MNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} )
      set(MNG_INCLUDE_DIR ${MNG_INCLUDE_DIRS} ) # for backward compatiblity
      set(MNG_LIBRARIES ${MNG_LIBRARY} ${ZLIB_LIBRARY})

      if (CYGWIN)
        if(BUILD_SHARED_LIBS)
           # No need to define MNG_USE_DLL here, because it's default for Cygwin.
        else()
          set (MNG_DEFINITIONS -DMNG_STATIC)
        endif()
      endif ()

  endif ()

  if (MNG_MNG_INCLUDE_DIR AND EXISTS "${MNG_MNG_INCLUDE_DIR}/mng.h")
      file(STRINGS "${MNG_MNG_INCLUDE_DIR}/mng.h" mng_version_str REGEX "^#define[ \t]+MNG_LIBMNG_VER_STRING[ \t]+\".+\"")

      string(REGEX REPLACE "^#define[ \t]+MNG_LIBMNG_VER_STRING[ \t]+\"([^\"]+)\".*" "\\1" MNG_VERSION_STRING "${mng_version_str}")
      unset(mng_version_str)
  endif ()
endif()

# handle the QUIETLY and REQUIRED arguments and set MNG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MNG
                                  REQUIRED_VARS MNG_LIBRARY MNG_MNG_INCLUDE_DIR
                                  VERSION_VAR MNG_VERSION_STRING)

mark_as_advanced(MNG_MNG_INCLUDE_DIR MNG_LIBRARY )
