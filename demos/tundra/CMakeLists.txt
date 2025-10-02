cmake_minimum_required(VERSION 3.16)
project(tundra C)

# Set C standard to C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set basic compiler flags (libraries will use these minimal flags)
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

# Use different optimization flags for CI vs local builds
if(DEFINED ENV{CI})
    # CI builds: use generic optimization for compatibility
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
else()
    # Local builds: use native optimization for best performance
    set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -march=native -g")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

# Disable tests, examples, and extra targets for all libraries
set(SDL_TEST OFF CACHE BOOL "Disable SDL tests")
set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests")
set(SDL_EXAMPLES OFF CACHE BOOL "Disable SDL examples")
set(SDL_INSTALL OFF CACHE BOOL "Disable SDL install")
add_subdirectory(lib/SDL)

# Configure shader-works to not build examples and enable threading
set(SHADER_WORKS_BUILD_EXAMPLES OFF CACHE BOOL "Don't build shader-works examples")
set(SHADER_WORKS_USE_THREADS ON CACHE BOOL "Enable threads")
add_subdirectory(lib/shader-works)

# Disable cJSON tests and utilities
set(ENABLE_CJSON_TEST OFF CACHE BOOL "Disable cJSON tests")
set(BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "Only build static lib")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Don't build shared lib")
add_subdirectory(lib/cJSON)

# Find source files in src directory
file(GLOB_RECURSE SOURCES "src/*.c")

# Only create executable if there are source files
if(SOURCES)
    add_executable(${PROJECT_NAME} ${SOURCES})

    # Link libraries
    target_link_libraries(${PROJECT_NAME}
        SDL3::SDL3
        shader-works-lib
        cjson
    )

    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE
        src
        lib/SDL/include
        lib/shader-works/include
        lib/cJSON
    )

    # Apply strict warning flags only to your project code
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wstrict-prototypes
        -Wshadow
    )

    # Copy config.json to build directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/res/config.json
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.json
    )
else()
    message(STATUS "No source files found in src/ directory")
    message(STATUS "Add .c files to src/ directory to build the main executable")
endif()

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)