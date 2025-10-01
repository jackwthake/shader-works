# MicroCraft Demo
# Multi-platform 3D voxel renderer showcasing the shader-works library

# Desktop build (SDL3) - Default
add_subdirectory(platforms/desktop)

# For embedded SAMD51 build:
# cd platforms/samd51 && mkdir build && cd build
# cmake -DCMAKE_TOOLCHAIN_FILE=../../arm-none-eabi-toolchain.cmake ..
# make
