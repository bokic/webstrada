#!/usr/bin/env bash

set -e

BUILD_UNIT_TESTS=OFF
CMAKE_ARGS=()

for arg in "$@"; do
    case "$arg" in
        --unit-tests|-u)
            BUILD_UNIT_TESTS=ON
            ;;
        --clean|-c)
            rm -rf build
            ;;
        *)
            CMAKE_ARGS+=("$arg")
            ;;
    esac
done

cmake -B build -G Ninja -DBUILD_UNIT_TESTS="${BUILD_UNIT_TESTS}" "${CMAKE_ARGS[@]}"
cmake --build build

cp build/compile_commands.json .
