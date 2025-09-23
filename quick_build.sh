#!/bin/bash

# Quick build script for single configuration
# Usage: ./quick_build.sh [debug|release] [threads|nothreads]

set -e

PROJECT_ROOT="$(dirname "$0")"
cd "$PROJECT_ROOT"

# Default values
BUILD_TYPE="Release"
USE_THREADS="ON"

# Parse arguments
case "${1,,}" in
    "debug"|"d") BUILD_TYPE="Debug" ;;
    "release"|"r") BUILD_TYPE="Release" ;;
    "") ;; # Use default
    *) echo "Invalid build type: $1. Use 'debug' or 'release'"; exit 1 ;;
esac

case "${2,,}" in
    "threads"|"t"|"on") USE_THREADS="ON" ;;
    "nothreads"|"nt"|"off") USE_THREADS="OFF" ;;
    "") ;; # Use default
    *) echo "Invalid threading option: $2. Use 'threads' or 'nothreads'"; exit 1 ;;
esac

BUILD_DIR="build"
THREADS_STR=$([ "$USE_THREADS" == "ON" ] && echo "with threading" || echo "single-threaded")

echo "Quick build: $BUILD_TYPE ($THREADS_STR)"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DSHADER_WORKS_USE_THREADS="$USE_THREADS" \
      ..

cmake --build . -j$(nproc)

echo
echo "Build complete! Run with: ./build/bin/basic_demo"