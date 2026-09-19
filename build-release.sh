#!/usr/bin/env bash

set -e

BUILD_DIR="build-release"

case "${1:-}" in
    "")
        ;;
    *)
        BUILD_DIR="$1"
        ;;
esac

echo ">> Configuring in ${BUILD_DIR}"
cmake -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++

echo ">> Building"
cmake --build "${BUILD_DIR}"

cp "${BUILD_DIR}/compile_commands.json" .

echo ">> Done."
