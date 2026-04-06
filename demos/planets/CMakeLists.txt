# Planets demo - integrated into shader-works build system
# 3D planet rendering demo showcasing the shader-works library

# Find source files in src directory
file(GLOB_RECURSE PLANETS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Noise sources from common
set(NOISE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/noise.c
)

# Only create executable if there are source files
if(PLANETS_SOURCES)
    add_executable(planets ${PLANETS_SOURCES} ${NOISE_SOURCES})

    # Link libraries (SDL3::SDL3 provided by parent, shader-works provided by parent)
    target_link_libraries(planets
        SDL3::SDL3
        shader-works
        demos-common
    )

    # Include directories
    target_include_directories(planets PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
    )

    # Apply strict warning flags only to planets code
    target_compile_options(planets PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wstrict-prototypes
        -Wshadow
        -O3
    )
endif()
