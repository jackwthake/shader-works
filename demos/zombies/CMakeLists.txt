# zombies demo - integrated into shader-works build system
# 3D planet rendering demo showcasing the shader-works library

# Find source files in src directory
file(GLOB_RECURSE ZOMBIES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Noise sources from common
set(NOISE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/noise.c
)

# Only create executable if there are source files
if(ZOMBIES_SOURCES)
    add_executable(zombies ${ZOMBIES_SOURCES} ${NOISE_SOURCES})

    # Link libraries (SDL3::SDL3 provided by parent, shader-works provided by parent)
    target_link_libraries(zombies
        SDL3::SDL3
        shader-works
        demos-common
    )

    # Include directories
    target_include_directories(zombies PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
    )

    # Apply strict warning flags only to zombies code
    target_compile_options(zombies PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wstrict-prototypes
        -Wshadow
        -O3
    )

    # Copy res folder to build/bin directory
    add_custom_command(TARGET zombies POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:zombies>/res"
        COMMENT "Copying zombies resources to build directory"
    )
endif()
