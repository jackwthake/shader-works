# Full test application
add_executable(full_test main.c shaders.c)

# Link to the cpu_render library
target_link_libraries(full_test PRIVATE cpu_render_lib)

# Set the executable name for clarity
set_target_properties(full_test PROPERTIES OUTPUT_NAME "full_test")