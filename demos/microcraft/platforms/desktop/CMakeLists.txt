# MicroCraft - Desktop Demo

add_executable(microcraft
    main.c
)

target_link_libraries(microcraft PRIVATE
    shader-works
    demos-common
    SDL3::SDL3
)

target_include_directories(microcraft PRIVATE
    ${CMAKE_SOURCE_DIR}/third-party/SDL/include
)
