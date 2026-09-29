# CPU Raytracer - Optimization Log
A single-threaded CPU raytracer, optimized step by step with measurements at each stage. Every row below is the same scene at the same resolution, so the times are directly comparable.

## Test machine
| | |
|---|---|
| CPU | AMD Ryzen 5 5600X 6-Core Processor |
| Compiler | c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |
| OS | Ubuntu 26.04.1 LTS |
| Scene | 7168 x 5376, unchanged since row 0 |

## Results
| Row | Change | Time (mean ± σ) | Speedup | IPC | Flags | imgdiff vs reference | 
|---|---|---|---|---|---|---|
| 0 | Baseline, no changes | 5.120 s ± 0.012 s | 1.00× | 2.06 | `-O3 -DNDEBUG` | - (this is the reference) | 
| 1 | No optimization | 103.515 s ± 0.961 s | 0.05× | 2.12 | `-O0` | 0 max Δ / 0 px |
| 2 | Standard optimization | 5.867 s ± 0.039 s | 0.87× | 1.97 | `-O2` | 0 max Δ / 0 px |
| 3 | Aggressive optimization | 5.157 s ± 0.020 s | 0.99× | 2.03 | `-O3` | 0 max Δ / 0 px |
| 4 | Target this CPU | 4.756 s ± 0.031 s | 1.08× | 1.89 | `-O3 -march=native` | 161 max Δ / 2326 px |

Speedup is row 0's time divided by this row's. Under 1.00× is a slowdown.

## Profile
Top three functions by self time, from `perf report --no-children --sort symbol` on the `build-prof` binary (`-O2 -g -fno-omit-frame-pointer`). Self time is samples where that function's own instructions were executing, excluding time inside its callees (it's what's actually worth optimizing).

| Function | Self time |
|---|---|
| `scene_intersect` | 63.72% |
| `cast_ray` | 14.66% |
| `render` | 6.37% |

Full graph: [`flame0.svg`](flame0.svg).

## Floating point
`-march=native` lets the compiler emit fused multiply-add, which computes `a*b + c` with one rounding instead of two. Results differ in the last bits. At 2326 pixels, that difference crossed an object boundary in the ray-sphere intersection test, flipping hit to miss, so those pixels changed shade. The image is not pixel-identical to the reference, but it is not less correct (FMA is the more accurate of the two).

## Method
Timings from `hyperfine --warmup 2 --runs 10`. Counters from `perf stat -e cycles,instructions`. IPC is (instructions / cycles). Correctness from `tools/imgdiff.py` against `reference.ppm` on every row; reported as max per-channel difference / differing pixel count.

One build directory per flag set, each configured with `-DCMAKE_BUILD_TYPE=None` so `CMAKE_CXX_FLAGS` is the complete flag set. Row 0 is the original `Release` build (`-O3 -DNDEBUG`).