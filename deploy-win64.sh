#!/bin/sh

# The release we are building
RELEASE="1.9.0"

PACKAGE="kail"

# Would create dependencies on dlls
GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DILUT_USE_SDL=FALSE -DILUT_USE_ALLEGRO=FALSE" 

# Segfaults
GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DIL_NO_MNG=ON" 

# Some useful libraries there
GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_SYSTEM_PREFIX_PATH=/mingw/x86_64-w64-mingw32"
GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_C_FLAGS=-m64"
GLOBAL_OPTIONS="$GLOBAL_OPTIONS -DCMAKE_CXX_FLAGS=-m64"

# try guessing where d3d is at
if [ -z "$DXSDK_DIR" ]; then
  export DXSDK_DIR="/C/Program Files (x86)/Microsoft SDKs/Windows/v7.1A"
fi

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

function build() {
  NAME="$1"
  OPTIONS="$2 $GLOBAL_OPTIONS"
  BUILD_PATH="$BASE_PATH/build/MinGW/$NAME"
  DEPLOY_PATH="$BASE_PATH/deploy/MinGW/$NAME"
  MSVC_PATH="$BASE_PATH/deploy/VC/$NAME"

  echo DEPLOY_PATH: $DEPLOY_PATH

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

      # create msvc target directories      
      mkdir -p "$MSVC_PATH/$BUILD/bin" "$MSVC_PATH/$BUILD/include" "$MSVC_PATH/$BUILD/lib" &&

      # build 
      cmake "$BASE_PATH" -G "MSYS Makefiles" $OPTIONS "-DCMAKE_INSTALL_PREFIX=$DEPLOY_PATH/$BUILD" "-DCMAKE_BUILD_TYPE=$BUILD" &&
      cmake --build . --target install --config "$BUILD" &&

      # add license
      cp "$BASE_PATH/COPYING" "$DEPLOY_PATH/$BUILD" &&

      # copy includes and dlls
      cp -r "$DEPLOY_PATH/$BUILD/include/"* "$MSVC_PATH/$BUILD/include" &&
      cp "$DEPLOY_PATH/$BUILD/bin/"*.dll "$MSVC_PATH/$BUILD/bin" &&

      # build import libraries for msvc
      cp "$DEPLOY_PATH/$BUILD/lib/"*.def "$MSVC_PATH/$BUILD/lib" &&
      ( 
        cd "$MSVC_PATH/$BUILD/lib" &&
        for DEF in *.def; do
          "$VC_LIB" /machine:x64 "/def:$DEF" || exit $?
        done
      ) || exit $?
    done
  ) || exit $?
}

build NonUnicode 
build Unicode    "-DIL_UNICODE=TRUE"  

for BUILD in NonUnicode Unicode; do
( 
  cd "$BASE_PATH/deploy/MinGW/$BUILD" &&
  tar czvf "$BASE_PATH/deploy/$PACKAGE-$RELEASE-x64-$BUILD-MinGW.tgz" Debug Release
  zip -r "$BASE_PATH/deploy/$PACKAGE-$RELEASE-x64-$BUILD-MinGW.zip" Debug Release
)

( 
  cd "$BASE_PATH/deploy/VC/$BUILD" &&
  zip -r "$BASE_PATH/deploy/$PACKAGE-$RELEASE-x64-$BUILD-VC.zip" Debug Release
)
done
