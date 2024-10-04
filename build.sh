#!/usr/bin/env bash

set -e

# rm -rf build

cmake -B build -G Ninja
cmake --build build

cp build/compile_commands.json .
