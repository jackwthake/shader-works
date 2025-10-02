cmake_minimum_required(VERSION 3.16)
project(shader-works C CXX)

# Set policies for compatibility
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Set proper compiler flags for build types (GCC/Clang)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")

    # Add -march=native only on x86/x64 platforms for better performance
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -march=native")
    endif()
elseif(MSVC)
    # MSVC compiler flags
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi")
endif()

# Set unified output directories in build/
set(UNIFIED_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
set(UNIFIED_LIB_DIR "${CMAKE_BINARY_DIR}/lib")

if(SHADER_WORKS_MULTI_CONFIG)
    # Multi-config: separate directories for each config
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${UNIFIED_BIN_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${UNIFIED_LIB_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${UNIFIED_LIB_DIR}")

    # Ensure directories exist for all configurations
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${UNIFIED_BIN_DIR}/${OUTPUTCONFIG}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${UNIFIED_LIB_DIR}/${OUTPUTCONFIG}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${UNIFIED_LIB_DIR}/${OUTPUTCONFIG}")
    endforeach()
else()
    # Single-config: all executables in unified bin directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${UNIFIED_BIN_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${UNIFIED_LIB_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${UNIFIED_LIB_DIR}")
endif()

# Workaround for cJSON's deprecated CMAKE_LEGACY_CYGWIN_WIN32 variable
# This was removed in CMake 3.27+, so we just ignore it
if(POLICY CMP0171)
    cmake_policy(SET CMP0171 NEW)
endif()

# SDL3 is needed for examples and desktop demos
option(SHADER_WORKS_BUILD_DEMOS "Build demo programs" ON)

if(SHADER_WORKS_BUILD_EXAMPLES OR SHADER_WORKS_BUILD_DEMOS)
    # Check if SDL exists as a subdirectory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/SDL/CMakeLists.txt")
        # Set SDL output directory to build/third-party
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/third-party")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/third-party")
        add_subdirectory(third-party/SDL EXCLUDE_FROM_ALL)
        # Restore output directories for main project
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${UNIFIED_LIB_DIR}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${UNIFIED_LIB_DIR}")
    else()
        # Try to find system SDL3
        find_package(SDL3 QUIET)
        if(NOT SDL3_FOUND)
            message(WARNING "SDL3 not found. Examples and desktop demos may not build correctly.")
        endif()
    endif()
endif()

# Create the shader-works library
add_library(shader-works STATIC
    lib/src/renderer.c
    lib/src/maths.c
    lib/src/primitives.c
    lib/src/shaders.c
)

# Set include directories for the library
target_include_directories(shader-works PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(shader-works PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/src
)

# Configure threading support
if(SHADER_WORKS_USE_THREADS)
    find_package(Threads REQUIRED)
    target_compile_definitions(shader-works PRIVATE SHADER_WORKS_USE_PTHREADS)
    target_link_libraries(shader-works PUBLIC Threads::Threads)
    message(STATUS "Threading support: ENABLED")
else()
    message(STATUS "Threading support: DISABLED")
endif()

# Core library only needs math library on Unix (including macOS)
if(UNIX)
    target_link_libraries(shader-works PUBLIC m)
endif()

# Add examples subdirectory
if(SHADER_WORKS_BUILD_EXAMPLES)
    add_subdirectory(examples/01_spinning_cube)
    add_subdirectory(examples/02_textured_scene)
    add_subdirectory(examples/03_fps_controller)
endif()

# Add desktop demos
if(SHADER_WORKS_BUILD_DEMOS)
    # Add tundra first (provides cJSON)
    add_subdirectory(demos/tundra)
    # Now add common library (depends on cJSON)
    add_subdirectory(demos/common)
    # Add microcraft
    add_subdirectory(demos/microcraft)
endif()
