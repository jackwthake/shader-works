#!/bin/bash

# Build script for multiple configurations
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
    rm -rf build-*
    print_status "All build directories cleaned"
fi

# Define build configurations
declare -A configs=(
    ["debug-threaded"]="Debug ON"
    ["debug-single"]="Debug OFF"
    ["release-threaded"]="Release ON"
    ["release-single"]="Release OFF"
)

print_header "Building multiple configurations"

# Build each configuration
for config_name in "${!configs[@]}"; do
    config_info=(${configs[$config_name]})
    build_type=${config_info[0]}
    use_threads=${config_info[1]}

    build_dir="build-${config_name}"

    print_status "Building ${config_name} (${build_type}, threads=${use_threads})"

    # Create and configure build directory
    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake -DCMAKE_BUILD_TYPE="$build_type" \
          -DSHADER_WORKS_USE_THREADS="$use_threads" \
          .. > /dev/null 2>&1

    # Build with progress info
    cmake --build . -j$(nproc) 2>&1 | grep -E "Built target|Linking|Building" || true

    # Verify build success
    if [[ -f "bin/basic_demo" && -f "lib/libshader-works-lib.a" ]]; then
        print_status "✓ ${config_name} build successful"
    else
        print_error "✗ ${config_name} build failed"
        exit 1
    fi

    cd ..
done

print_header "Build Summary"
echo -e "${GREEN}All configurations built successfully!${NC}"
echo
echo "Build directories:"
for config_name in "${!configs[@]}"; do
    build_dir="build-${config_name}"
    if [[ -d "$build_dir" ]]; then
        exe_size=$(du -h "$build_dir/bin/basic_demo" | cut -f1)
        lib_size=$(du -h "$build_dir/lib/libshader-works-lib.a" | cut -f1)
        echo "  $build_dir: executable=${exe_size}, library=${lib_size}"
    fi
done

echo
echo "To test a specific configuration:"
echo "  ./build-debug-threaded/bin/basic_demo"
echo "  ./build-release-single/bin/basic_demo"
echo
echo "To clean all builds: ./build_all.sh clean"