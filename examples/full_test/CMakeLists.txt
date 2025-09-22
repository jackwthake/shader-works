# Full test application
add_executable(full_test main.c shaders.c)

# Link to the cpu_render library and SDL3
target_link_libraries(full_test PRIVATE cpu_render_lib SDL3::SDL3)

# Include SDL headers
target_include_directories(full_test PRIVATE ${CMAKE_SOURCE_DIR}/examples/lib/SDL/include)

# Set the executable name for clarity
set_target_properties(full_test PROPERTIES OUTPUT_NAME "full_test")