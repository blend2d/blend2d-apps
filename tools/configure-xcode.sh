#!/bin/sh

BUILD_OPTIONS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

if [ -n "$VCPKG_ROOT" ]; then
  BUILD_OPTIONS="${BUILD_OPTIONS} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
fi

cmake .. -B ../build_xcode -G"Xcode" ${BUILD_OPTIONS} "$@"
