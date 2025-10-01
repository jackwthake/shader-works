# 03 - FPS Controller
add_executable(03_fps_controller main.c)

# Link to the shader-works library and SDL3
target_link_libraries(03_fps_controller PRIVATE shader-works SDL3::SDL3)

# Include SDL headers
target_include_directories(03_fps_controller PRIVATE ${CMAKE_SOURCE_DIR}/third-party/SDL/include)
