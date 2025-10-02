# Common library for demos
# Shared code used by multiple demo applications

add_library(demos-common STATIC
    chunk_map.c
    config.c
    noise.c
)

# Public include directory for other demos to use
# Set the parent directory so includes are like "common/file.h"
target_include_directories(demos-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link against shader-works (header-only dependency for chunk_map)
target_link_libraries(demos-common PUBLIC
    shader-works
)

# cJSON is needed by config.c
# We need to add the include path for cJSON. Tundra provides it.
if(TARGET cjson)
    # Get the cJSON include directory from the tundra submodule
    target_include_directories(demos-common PUBLIC
        ${CMAKE_SOURCE_DIR}/demos/tundra/third-party/cJSON
    )
endif()
