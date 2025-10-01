#!/bin/bash

# Build script for all projects (examples and demos)
# Usage: ./build_all.sh [clean]

set -e  # Exit on any error

PROJECT_ROOT="$(dirname "$0")"
cd "$PROJECT_ROOT"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}======================================${NC}"
}

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Clean build directories if requested
if [[ "$1" == "clean" ]]; then
    print_header "Cleaning build directories"
    rm -rf build
    rm -rf demos/microcraft/build
    print_status "All build directories cleaned"
fi

# Build main project (library, examples, and desktop demos)
print_header "Building main project"

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. > /dev/null
cmake --build . -j$(nproc)

if [[ -f "lib/libshader-works.a" ]]; then
    print_status "✓ Main project build successful"
else
    print_error "✗ Main project build failed"
    exit 1
fi

cd ..

# Note: MicroCraft embedded (SAMD51) build must be done separately
# Run: cd demos/microcraft/platforms/samd51 && mkdir build && cd build
#      cmake -DCMAKE_TOOLCHAIN_FILE=../../arm-none-eabi-toolchain.cmake ..
#      make

print_header "Build Summary"
echo -e "${GREEN}All projects built successfully!${NC}"
echo
echo "Executables:"
if [[ -d "build/bin" ]]; then
    for exe in build/bin/*; do
        if [[ -f "$exe" ]]; then
            size=$(du -h "$exe" | cut -f1)
            echo "  - $(basename "$exe"): ${size}"
        fi
    done
fi

echo
echo "Run examples and demos:"
echo "  ./build/bin/01_spinning_cube"
echo "  ./build/bin/02_textured_scene"
echo "  ./build/bin/03_fps_controller"
echo "  ./build/bin/microcraft"
echo
echo "To clean all builds: ./build_all.sh clean"