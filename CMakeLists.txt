cmake_minimum_required(VERSION 3.16)
project(shader-works C)

# Set the output directory for built objects.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_RelWithDebInfo "-O3 -march=native -g")

# SDL is only needed for examples
add_subdirectory(examples/lib/SDL EXCLUDE_FROM_ALL)

# Create the shader-works library
add_library(shader-works-lib STATIC
    src/renderer.c
    src/maths.c
    src/primitives.c
    src/shaders.c
)

# Set include directories for the library
target_include_directories(shader-works-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(shader-works-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Core library only needs math library
target_link_libraries(shader-works-lib PUBLIC m)

# Add examples subdirectory
add_subdirectory(examples/basic_demo)
add_subdirectory(examples/full_test)