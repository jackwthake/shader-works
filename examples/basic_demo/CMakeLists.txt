# Basic demo application
add_executable(basic_demo main.c)

# Link to the shader-works library and SDL3
target_link_libraries(basic_demo PRIVATE shader-works-lib SDL3::SDL3)

# Include SDL headers
target_include_directories(basic_demo PRIVATE ${CMAKE_SOURCE_DIR}/examples/lib/SDL/include)

# Set the executable name for clarity
set_target_properties(basic_demo PROPERTIES OUTPUT_NAME "basic_demo")