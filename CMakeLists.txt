cmake_minimum_required(VERSION 3.16)
project(shader-works C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configuration options
option(SHADER_WORKS_USE_THREADS "Enable threading support" ON)
option(SHADER_WORKS_MULTI_CONFIG "Build multiple configurations" OFF)
option(SHADER_WORKS_BUILD_EXAMPLES "Build example programs" ON)

# Configure build types with standard naming
if(SHADER_WORKS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
    message(STATUS "Multi-config build enabled: Debug, Release, RelWithDebInfo")
else()
    # Single config build - use CMAKE_BUILD_TYPE
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    endif()
    message(STATUS "Single-config build: ${CMAKE_BUILD_TYPE}")
endif()

# Set proper compiler flags for build types
set(CMAKE_C_FLAGS_Debug "-g -O0")
set(CMAKE_C_FLAGS_Release "-O3 -march=native -DNDEBUG")
set(CMAKE_C_FLAGS_RelWithDebInfo "-O3 -march=native -g")

# Set output directories
if(SHADER_WORKS_MULTI_CONFIG)
    # Multi-config: separate directories for each config
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

    # Ensure directories exist for all configurations
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG}")
    endforeach()
else()
    # Single-config: simple directory structure
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

# SDL is only needed for examples
if(SHADER_WORKS_BUILD_EXAMPLES)
    add_subdirectory(examples/lib/SDL EXCLUDE_FROM_ALL)
endif()

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

# Configure threading support
if(SHADER_WORKS_USE_THREADS)
    target_compile_definitions(shader-works-lib PRIVATE SHADER_WORKS_USE_PTHREADS)
    target_link_libraries(shader-works-lib PUBLIC pthread)
    message(STATUS "Threading support: ENABLED")
else()
    message(STATUS "Threading support: DISABLED")
endif()

# Core library only needs math library
target_link_libraries(shader-works-lib PUBLIC m)

# Add examples subdirectory
if(SHADER_WORKS_BUILD_EXAMPLES)
    add_subdirectory(examples/basic_demo)
    add_subdirectory(examples/full_test)
    add_subdirectory(examples/fps_controller)
endif()
