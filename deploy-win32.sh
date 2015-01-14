#!/bin/sh
#
# This is a helper script for me to create precompiled binary packages for
# the Windows operating system. It is written to match my build environment(s)
# so it might not be useful to anyone else.
#
# It uses the following environment variables:
# 
#   GLOBAL_OPTIONS            Any option going to cmake.
#   BITNESS                   Defines the target architecture. 32 (default) or 64
#   VC_PATH                   Base directory of Visual Studio
#

# The release we are building
RELEASE="1.11.0"
PACKAGE="kail"

# Would create dependencies on dlls
GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DILUT_USE_SDL=FALSE -DILUT_USE_SDL2=FALSE -DILUT_USE_ALLEGRO=FALSE" 

# Defined based on the selected toolchain
case $BITNESS in
  64)
    # Segfaults
    GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DIL_NO_MNG=ON" 

    # Some useful libraries there
    GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_SYSTEM_PREFIX_PATH=/mingw/x86_64-w64-mingw32"

    # 64 bit architecture
    GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_C_FLAGS=-m64"
    GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_CXX_FLAGS=-m64"

    ARCH="x64"
  ;;

  *)
    # Some useful libraries there
    GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_SYSTEM_PREFIX_PATH=/mingw/i686-w64-mingw32"

    ARCH="x86"
  ;;
esac

# try guessing where msvc is
if [ -z "$VC_PATH" ]; then
  VC_PATH="/C/Program Files (x86)/Microsoft Visual Studio 11.0"

  if [ ! -d "$VC_PATH" ]; then
	 VC_PATH="/E/vs"
  fi
fi

BASE_PATH=`pwd`

export MAKEFLAGS="-j9"

# add some msvc tools to the path
PATH="$PATH:$VC_PATH/VC/bin:$VC_PATH/Common7/IDE"

# msvc library tool
VC_LIB=lib.exe

# TODO: check if lib.exe can be run, if not, disable building VC package

if ! $VC_LIB -NOLOGO ; then
  echo "Warning: $VC_LIB could not be executed, disabling VC package building"
  VC_LIB=
fi

function build() {
  NAME="$1"
  OPTIONS="$2 $GLOBAL_OPTIONS"
  BUILD_PATH="$BASE_PATH/build/MinGW/$ARCH/$NAME"
  DEPLOY_PATH="$BASE_PATH/deploy/MinGW/$ARCH/$NAME"
  MSVC_PATH="$BASE_PATH/deploy/VC/$ARCH/$NAME"

  echo "BUILD_PATH:   $BUILD_PATH"
  echo "DEPLOY_PATH:  $DEPLOY_PATH"
  echo "MSVC_PATH:    $MSVC_PATH"

  # initialize deployment path
  rm -rf "$DEPLOY_PATH" &&
  rm -rf "$MSVC_PATH" &&

  # create build environment
  mkdir -p  "$BUILD_PATH" || exit $?
  (
    cd "$BUILD_PATH" &&

    for BUILD in Release Debug; do
      # create mingw target directories      
      mkdir -p  "$DEPLOY_PATH/$BUILD" || exit $?

      # build 
      cmake "$BASE_PATH" -G "MSYS Makefiles" $OPTIONS "-DCMAKE_INSTALL_PREFIX=$DEPLOY_PATH/$BUILD" "-DCMAKE_BUILD_TYPE=$BUILD" &&
      cmake --build . --target install --config "$BUILD" &&

      # add license and changelog
      cp "$BASE_PATH/ChangeLog"     "$DEPLOY_PATH/$BUILD" &&
      cp "$BASE_PATH/COPYING"       "$DEPLOY_PATH/$BUILD" &&
      cp "$BASE_PATH/COPYING.libs"  "$DEPLOY_PATH/$BUILD" &&

      # build visual studio libs
      if [ ! -z "$VC_LIB" ]; then
        mkdir -p  "$MSVC_PATH/$BUILD" &&

        # add license and changelog
        cp "$BASE_PATH/ChangeLog"     "$MSVC_PATH/$BUILD" &&
        cp "$BASE_PATH/COPYING"       "$MSVC_PATH/$BUILD" &&
        cp "$BASE_PATH/COPYING.libs"  "$MSVC_PATH/$BUILD" &&

        # create msvc target directories      
        mkdir -p  "$MSVC_PATH/$BUILD/bin" "$MSVC_PATH/$BUILD/include" "$MSVC_PATH/$BUILD/lib" &&

        # copy includes and dlls
        cp -r "$DEPLOY_PATH/$BUILD/include/"* "$MSVC_PATH/$BUILD/include" &&
        cp    "$DEPLOY_PATH/$BUILD/bin/"*.dll "$MSVC_PATH/$BUILD/bin"     &&

        # build import libraries for msvc
        cp "$DEPLOY_PATH/$BUILD/lib/"*.def    "$MSVC_PATH/$BUILD/lib"     &&
        ( 
          cd "$MSVC_PATH/$BUILD/lib" &&
          for DEF in *.def; do
            "$VC_LIB" "-machine:$ARCH" "-def:$DEF" || exit $?
          done
        ) || exit $?
      fi 
    done
  ) || exit $?
}

build NonUnicode 
build Unicode    "-DIL_UNICODE=TRUE"  

for BUILD in NonUnicode Unicode; do
( 
  cd "$BASE_PATH/deploy/MinGW/$ARCH/$BUILD" &&
  tar czvf "$BASE_PATH/deploy/$PACKAGE-$RELEASE-$ARCH-$BUILD-MinGW.tgz" Debug Release
  zip -r "$BASE_PATH/deploy/$PACKAGE-$RELEASE-$ARCH-$BUILD-MinGW.zip" Debug Release
)

if [ ! -z "$VC_LIB" ]; then
( 
  cd "$BASE_PATH/deploy/VC/$ARCH/$BUILD" &&
  zip -r "$BASE_PATH/deploy/$PACKAGE-$RELEASE-$ARCH-$BUILD-VC.zip" Debug Release
)
fi

done
