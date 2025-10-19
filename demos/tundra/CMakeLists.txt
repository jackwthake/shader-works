# Tundra demo - integrated into shader-works build system
# This CMakeLists.txt is designed to work as part of the shader-works project

# Add cJSON library (only if not already added)
if(NOT TARGET cjson)
    set(ENABLE_CJSON_TEST OFF CACHE BOOL "Disable cJSON tests")
    set(BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "Only build static lib")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Don't build shared lib")

    # Apply patch to fix cJSON CMake compatibility
    execute_process(
        COMMAND patch -p0 -N -i ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cjson-cmake-fix.patch
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cJSON
        RESULT_VARIABLE PATCH_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(WIN32)
        message(STATUS "Disabling /WX (treat warnings as errors) for cJSON on Windows")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /D_CRT_SECURE_NO_WARNINGS") # optional silencing
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /WX-")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /WX-")
        set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /WX-")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /WX-")
    endif()

    add_subdirectory(third-party/cJSON)
endif()

# Find source files in src directory
file(GLOB_RECURSE TUNDRA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Only create executable if there are source files
if(TUNDRA_SOURCES)
    add_executable(tundra ${TUNDRA_SOURCES})

    # Link libraries (SDL3::SDL3 provided by parent, shader-works renamed to shader-works)
    target_link_libraries(tundra
        SDL3::SDL3
        shader-works
        demos-common
        cjson
    )

    # Include directories
    target_include_directories(tundra PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cJSON
    )

    # Apply strict warning flags only to your project code
    target_compile_options(tundra PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wstrict-prototypes
        -Wshadow
    )

    # Copy config.json to build directory
    add_custom_command(TARGET tundra POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/res/config.json
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.json
    )
else()
    message(STATUS "No source files found in demos/tundra/src/ directory")
endif()
