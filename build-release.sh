#!/usr/bin/env bash

# Release build with heavy size optimizations:
#   -O3
#   -ffunction-sections/-fdata-sections + --gc-sections  (drop dead code from static libs)
#   --icf=safe        (fold byte-identical functions into one copy each; gold/lld only)
#   -s                (strip symbol tables)
#
# Optional LTO (slower build, more shrinking):
#   WITH_LTO=1 ./build-release.sh
# or:
#   ./build-release.sh --lto

set -e

BUILD_DIR="build-release"
EXTRA_CMAKE_ARGS=()

case "$1" in
    --lto|--thin-lto) EXTRA_CMAKE_ARGS+=(-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE) ;;
    "") ;;
    *) BUILD_DIR="$1" ;;
esac

if [ -n "${WITH_LTO:-}" ] && [ "${WITH_LTO:-}" != "0" ]; then
    EXTRA_CMAKE_ARGS+=(-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE)
fi

CFLAGS="-O3 -ffunction-sections -fdata-sections"
LDFLAGS="-fuse-ld=gold -Wl,--gc-sections -Wl,--icf=safe -Wl,-s"

echo ">> Configuring in ${BUILD_DIR}"
cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CFLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS" \
    -DCMAKE_SHARED_LINKER_FLAGS="$LDFLAGS" \
    "${EXTRA_CMAKE_ARGS[@]}"

echo ">> Building"
cmake --build "$BUILD_DIR"

cp "$BUILD_DIR/compile_commands.json" .

echo ">> Done."
