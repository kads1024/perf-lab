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
| Row | Change | Time (mean ± σ) | IPC | Flags | imgdiff vs reference |
|---|---|---|---|---|---|
| 0 | Baseline, no changes | 5.120 s ±  0.012 s | 2.06 | `-O3 -DNDEBUG` | - (this is the reference) |

## Method 
Timings from `hyperfine --warmup 2 --runs 10`. Counters from `perf stat -e cycles,instructions`. IPC is (instructions / cycles).
