# Basic demo application
add_executable(basic_demo main.c)

# Link to the cpu_render library
target_link_libraries(basic_demo PRIVATE cpu_render_lib)

# Set the executable name for clarity
set_target_properties(basic_demo PROPERTIES OUTPUT_NAME "basic_demo")