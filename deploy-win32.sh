#!/bin/sh

# try guessing where d3d is at
if [ -z "$DXSDK_DIR" ]; then
  export DXSDK_DIR="/C/Program Files (x86)/Microsoft SDKs/Windows/v7.1A"
fi

# I have some libraries there
GLOBAL_OPTIONS="-DCMAKE_SYSTEM_PREFIX_PATH=/mingw/i686-w64-mingw32"

function build() {
  SYSTEM="$1"
  NAME="$2"
  OPTIONS="$3"
  PACK="$4"
  PACKEXT="$5"

  OPTIONS="$OPTIONS $GLOBAL_OPTIONS"
  BASE_PATH=`pwd`
  BUILD_PATH="$BASE_PATH/build/$SYSTEM/$NAME"
  DEPLOY_PATH="$BASE_PATH/deploy/$SYSTEM/$NAME"

  echo DEPLOY_PATH: $DEPLOY_PATH

  # initialize deployment path
  rm    -rf "$DEPLOY_PATH" &&
  mkdir -p  "$DEPLOY_PATH/Release" "$DEPLOY_PATH/Debug" || exit $?

  # create build environment
  mkdir -p  "$BUILD_PATH" || exit $?
  (
    cd "$BUILD_PATH" &&
    cmake "$BASE_PATH" -G "$SYSTEM" $OPTIONS "-DCMAKE_INSTALL_PREFIX=$DEPLOY_PATH/Release" "-DCMAKE_BUILD_TYPE=Release" &&
    cmake --build . --target install --config "Release" &&
    cmake "$BASE_PATH" -G "$SYSTEM" $OPTIONS "-DCMAKE_INSTALL_PREFIX=$DEPLOY_PATH/Debug" "-DCMAKE_BUILD_TYPE=Debug" &&
    cmake --build . --target install --config "Debug"
  ) || exit $?

  (
    cd "$DEPLOY_PATH" && 
    $PACK "$BASE_PATH/KaIL-$SYSTEM-$NAME.$PACKEXT" Release Debug 
  )
}

build "Visual Studio 11" NonUnicode ""                  "zip -r"   ".zip"   &&
build "Visual Studio 11" Unicode    "-DIL_UNICODE=TRUE" "zip -r"   ".zip"   &&
build "MSYS Makefiles"   NonUnicode ""                  "tar cjvf" ".tbz2"  &&
build "MSYS Makefiles"   Unicode    "-DIL_UNICODE=TRUE" "tar cjvf" ".tbz2"  
