# 02 - Textured Scene
add_executable(02_textured_scene main.c shaders.c)

# Link to the shader-works library and SDL3
target_link_libraries(02_textured_scene PRIVATE shader-works SDL3::SDL3)

# Include SDL headers
target_include_directories(02_textured_scene PRIVATE ${CMAKE_SOURCE_DIR}/third-party/SDL/include)