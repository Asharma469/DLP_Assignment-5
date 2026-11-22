#!/bin/bash
set -euo pipefail

echo "Compiling DLP Image Processing Benchmarks..."

# Scalar baseline: exclude AVX2 and intrinsic mode support.
gcc -O3 -march=x86-64 -msse2 -mno-avx -mno-avx2 \
  dlp_image_bench.c -o dlp_image_bench_scalar -lm

# gem5-compatible explicit SIMD binary.
# AVX avoids vbroadcastss (0F38/18), which this gem5 X86 build may not decode.
gcc -O3 -mavx -fno-tree-vectorize \
  dlp_image_bench.c -DDLP_AVX2 \
  -o dlp_image_bench_avx2_gem5 -lm

# OpenMP compiler-vectorized binary.
gcc -O3 -march=x86-64 -msse2 -mno-avx -mno-avx2 -fopenmp \
  dlp_image_bench.c -o dlp_image_bench_openmp -lm

echo "Compilation complete."
