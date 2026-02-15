#!/bin/bash

# Exit on any error
set -e

# Configuration
PREFIX_PATH="$HOME/my_libs"
BUILD_TYPE="Release" # Change to "Release" as needed

# 1. Path to Zeta (assumes Zeta and Iota are siblings)
ZETA_DIR="../../Zeta/Zeta"
IOTA_DIR="."

echo "--- Building and Installing Zeta Library ---"
cd "$ZETA_DIR"
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX="$PREFIX_PATH"
cmake --build build_release --target install

echo "--- Building Iota App ---"
cd - > /dev/null # Go back to Iota directory
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_PREFIX_PATH="$PREFIX_PATH"
cmake --build build_release

echo "--- Build Complete! ---"
echo "Run with: ./build_release/Iota"
