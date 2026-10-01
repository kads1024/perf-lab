for t in 1 2 4 6 8 12; do
  echo "=== $t threads ==="
  OMP_NUM_THREADS=$t hyperfine --warmup 2 --runs 5 "./build-omp/mini-raytracer"
done
