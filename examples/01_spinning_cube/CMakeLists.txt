# 01 - Spinning Cube
add_executable(01_spinning_cube main.c)

# Link to the shader-works library and SDL3
target_link_libraries(01_spinning_cube PRIVATE shader-works SDL3::SDL3)

# Include SDL headers
target_include_directories(01_spinning_cube PRIVATE ${CMAKE_SOURCE_DIR}/third-party/SDL/include)