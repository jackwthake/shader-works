# 04 - Benchmark

Performance benchmark for the shader-works renderer.

## What it does

Renders spheres with increasing complexity (32 to 8192 triangles) and measures:
- Frames per second (FPS)
- Triangles rendered per second
- Total render time

Results are written to `benchmark_results.csv` for analysis.

## Building

```bash
# From project root
./quick_build.sh release threads

# Or with CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DSHADER_WORKS_USE_THREADS=ON ..
cmake --build . --target 04_benchmark
```

## Running

```bash
# From build directory
./bin/04_benchmark

# Output: benchmark_results.csv
```

## Comparing threaded vs single-threaded

Build twice with different thread settings:

```bash
# Threaded build
cmake -DSHADER_WORKS_USE_THREADS=ON .. && cmake --build . --target 04_benchmark
./bin/04_benchmark
mv benchmark_results.csv benchmark_threaded.csv

# Single-threaded build
cmake -DSHADER_WORKS_USE_THREADS=OFF .. && cmake --build . --target 04_benchmark
./bin/04_benchmark
mv benchmark_results.csv benchmark_single.csv
```

Then compare the `tri_per_sec` column to see threading benefits.
