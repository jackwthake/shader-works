# MicroCraft - Desktop Demo

add_executable(microcraft
    main.cpp
    ../../src/scene.cpp
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
