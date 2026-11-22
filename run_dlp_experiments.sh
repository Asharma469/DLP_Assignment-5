#!/bin/bash
set -euo pipefail

ITERATIONS="${ITERATIONS:-50}"

echo "========================================================="
echo "   Running DLP Data-Level Parallelism Benchmarks        "
echo "========================================================="

for entry in \
  "Scalar|dlp_image_bench_scalar|0" \
  "AVX SIMD|dlp_image_bench_avx2_gem5|1" \
  "OpenMP SIMD|dlp_image_bench_openmp|2"
do
  IFS='|' read -r name binary mode <<< "$entry"

  if [[ ! -x "./$binary" ]]; then
    echo "Missing executable: ./$binary" >&2
    echo "Run ./build_dlp_benchmarks.sh first." >&2
    exit 1
  fi

  echo "[${mode}] Running ${name}..."
  "./$binary" "$mode" "$ITERATIONS"
done

echo "All native DLP benchmark runs completed successfully."
