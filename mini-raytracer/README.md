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
| Row | Change | Time (mean ± σ) | Speedup | IPC | Flags | imgdiff vs reference | Note |
|---|---|---|---|---|---|---|---|
| 0 | Baseline, no changes | 5.120 s ± 0.012 s | 1.00× | 2.06 | `-O3 -DNDEBUG` | — (this is the reference) | Day 1 baseline |
| 1 | No optimization | 103.515 s ± 0.961 s | 0.05× | 2.12 | `-O0` | 0 max Δ / 0 px | 20× slower than row 0. Every value round-trips through memory and nothing is inlined, so the instruction count explodes. IPC is unchanged (2.12 vs 2.06) — the extra instructions are trivial and independent, so the pipeline stays just as full while doing far more work. IPC alone would not have told me this build was 20× slower. |
| 2 | Standard optimization | 5.867 s ± 0.039 s | 0.87× | 1.97 | `-O2` | 0 max Δ / 0 px | 11.4% more cycles than `-O3`, from 8.1% more instructions (52.78 B vs 48.84 B) at 3% lower IPC. `-O3` inlines more and unrolls more, removing call overhead and loop bookkeeping; the resulting drop in branches also explains its IPC edge. Both effects push the same way. |
| 3 | Aggressive optimization | 5.157 s ± 0.020 s | 0.99× | 2.03 | `-O3` | 0 max Δ / 0 px | Matches row 0 to within 0.7%, inside run-to-run noise. Confirms row 0 was already a fully optimized build and that `-DNDEBUG` costs nothing here — no `assert()` in the hot path. |
| 4 | Target this CPU | 4.756 s ± 0.031 s | 1.08× | 1.89 | `-O3 -march=native` | 161 max Δ / 2326 px | 15.1% fewer instructions (41.45 B vs 48.84 B) at 7.1% lower IPC, netting 8.6% fewer cycles. Zen 3 AVX2 and FMA pack the same work into fewer, heavier instructions: each retires more slowly, so IPC falls while total time improves. See "Floating point" below. |

Speedup is row 0's time divided by this row's. Under 1.00× is a slowdown.

## Profile
Top three functions by self time, from `perf report --no-children --sort symbol` on the `build-prof` binary (`-O2 -g -fno-omit-frame-pointer`). Self time is samples where that function's own instructions were executing, excluding time inside its callees (it's what's actually worth optimizing).

| Function | Self time |
|---|---|
| `scene_intersect` | 63.72% |
| `cast_ray` | 14.66% |
| `render` | 6.37% |

Full graph: [`flame0.svg`](flame0.svg). These three are the Week 2 targets.

## Floating point
`-march=native` lets the compiler emit fused multiply-add, which computes `a*b + c` with one rounding instead of two. Results differ in the last bits. At 2326 pixels, that difference crossed an object boundary in the ray-sphere intersection test, flipping hit to miss, so those pixels changed shade. The image is not pixel-identical to the reference, but it is not less correct (FMA is the more accurate of the two).

## Method
Timings from `hyperfine --warmup 2 --runs 10`. Counters from `perf stat -e cycles,instructions`. IPC is (instructions / cycles). Correctness from `tools/imgdiff.py` against `reference.ppm` on every row; reported as max per-channel difference / differing pixel count.

One build directory per flag set, each configured with `-DCMAKE_BUILD_TYPE=None` so `CMAKE_CXX_FLAGS` is the complete flag set. Row 0 is the original `Release` build (`-O3 -DNDEBUG`).

Times and cycle counts come from separate runs (`hyperfine` and `perf stat`), so they diverge a little: `-O2` is 13.8% slower than `-O3` by wall time but only 11.4% by cycles. Without frequency and boost control (Day 5) the CPU clock is free to move between runs, so cycles are the more trustworthy of the two until then. IPC in the table is computed from the `perf stat` pair shown, not averaged across runs.

**Caveats, current as of Day 2:**
- No CPU frequency or boost control yet (scheduled Day 5). σ here is larger than it will be from Friday onward, and all rows will be re-measured once that's in place.
- The profiled binary is not the timed binary. Profiling uses `-O2 -g -fno-omit-frame-pointer` so stacks resolve; the timed rows use the flags listed per row.
- `perf script --no-inline` was required: this perf build's DWARF reader rejects some of GCC 15's debug info. Inline frames are folded into their callers in `flame0.svg`.