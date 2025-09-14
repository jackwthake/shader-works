cmake_minimum_required(VERSION 3.16)
project(cpu_render)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in lib/SDL
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# Create your game executable target as usual
add_executable(cpu_render src/main.c)
set(EXECUTABLE_NAME ${PROJECT_NAME})

# Link to the actual SDL3 library.
target_link_libraries(cpu_render PRIVATE SDL3::SDL3)