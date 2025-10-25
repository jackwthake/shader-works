# 04 - Benchmark
add_executable(04_benchmark main.c)

# Link to the shader-works library (no SDL needed)
target_link_libraries(04_benchmark PRIVATE shader-works)
