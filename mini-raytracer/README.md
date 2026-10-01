# CPU Raytracer - Optimization Log
A CPU raytracer, optimized step by step with measurements at each stage. Rows in the Results table are all single-threaded at the same scene and resolution, so they are directly comparable; threading results are measured separately in the Threading section.

## Test machine
| | |
|---|---|
| CPU | AMD Ryzen 5 5600X 6-Core Processor (znver3) |
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

Speedup is row 0's time divided by this row's. Under 1.00× is a slowdown. All rows are single-threaded and are not comparable to the threaded times in the Threading section.

## Profile
Top three functions by self time, from `perf report --no-children --sort symbol` on the `build-prof` binary (`-O2 -g -fno-omit-frame-pointer`). Self time is samples where that function's own instructions were executing, excluding time inside its callees (it's what's actually worth optimizing).

| Function | Self time |
|---|---|
| `scene_intersect` | 63.72% |
| `cast_ray` | 14.66% |
| `render` | 6.37% |

Full graph: [`flame0.svg`](flame0.svg).

## Threading

The render loop is parallelized with `#pragma omp parallel for schedule(dynamic, 1)` over image rows. Dynamic scheduling is used because rows are not equally expensive (rows crossing the glass sphere spawn recursive reflection and refraction rays, while background rows return immediately). Output is pixel-identical to `reference.ppm` (imgdiff 0 max Δ / 0 px), so the parallel version is correct.

![scaling](scaling.png)

Measured speedup reaches 3.07× at 6 threads against an ideal 6×, and only 3.43× at 12. The curve falls away from ideal immediately (1.64× at 2 threads) so hyperthreading is not the main cause; it cannot apply below the physical core count. The dominant cause is the serial fraction. Timing the three phases separately, frameBuffer allocation (which zero-fills 460 MB) and the single-threaded PPM write take 985ms of the 5213ms single-threaded total, or 18.9%. Neither shrinks with thread count: going from 1 to 12 threads, the render loop drops from 4228 ms to 532 ms while alloc and write stay at 164/821 ms and 171/808 ms, leaving the serial portion at 64.8% of the 12-thread run. By Amdahl's law an 18.9% serial fraction caps speedup at about 5.3× regardless of core count. Hyperthreading explains the further flattening past 6 threads, where additional logical threads share execution units with a sibling rather than getting their own core. Turbo frequency likely falls as more cores wake and would flatten the curve further, but frequency control isn't in place yet, so I can't separate it from the other two.

## Floating point
`-march=native` lets the compiler emit fused multiply-add, which computes `a*b + c` with one rounding instead of two. Results differ in the last bits. At 2326 pixels, that difference crossed an object boundary in the ray-sphere intersection test, flipping hit to miss, so those pixels changed shade. The image is not pixel-identical to the reference, but it is not less correct (FMA is the more accurate of the two).

## Method
Timings from `hyperfine --warmup 2 --runs 10`. Counters from `perf stat -e cycles,instructions`. IPC is (instructions / cycles). Correctness from `tools/imgdiff.py` against `reference.ppm` on every row; reported as max per-channel difference / differing pixel count.

One build directory per flag set, each configured with `-DCMAKE_BUILD_TYPE=None` so `CMAKE_CXX_FLAGS` is the complete flag set. Row 0 is the original `Release` build (`-O3 -DNDEBUG`).

Phase timings (alloc / render / write) come from `std::chrono::steady_clock` around each section of `render()`, printed to stderr on every run.