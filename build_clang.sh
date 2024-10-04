#!/usr/bin/env bash

set -e

# rm -rf build

cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build

cp build/compile_commands.json .
