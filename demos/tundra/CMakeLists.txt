# Tundra demo - integrated into shader-works build system
# This CMakeLists.txt is designed to work as part of the shader-works project

# Add cJSON library (only if not already added)
if(NOT TARGET cjson)
    set(ENABLE_CJSON_TEST OFF CACHE BOOL "Disable cJSON tests")
    set(BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "Only build static lib")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Don't build shared lib")
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