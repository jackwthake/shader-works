# MicroCraft - Desktop Demo

# Generate resources.inl from resource files
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../../src/resources.inl
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/bake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../res/atlas.bmp ${CMAKE_CURRENT_SOURCE_DIR}/../../res/test.bmp
    COMMENT "Generating resources.inl from resource files"
)

add_executable(microcraft
    main.cpp
    ../../src/scene.cpp
    ../../src/resources.inl  # Add as dependency so it gets regenerated
)

target_link_libraries(microcraft PRIVATE
    shader-works
    demos-common
    SDL3::SDL3
)

target_include_directories(microcraft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/third-party/SDL/include
)
