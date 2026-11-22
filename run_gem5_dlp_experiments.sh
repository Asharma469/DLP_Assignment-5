#!/bin/bash
set -euo pipefail

GEM5_BIN="${GEM5_BIN:-$HOME/gem5/build/X86/gem5.opt}"
OUT_DIR="${OUT_DIR:-results_dlp}"
ITERATIONS="${ITERATIONS:-50}"

mkdir -p "$OUT_DIR"

if [[ ! -x "$GEM5_BIN" ]]; then
  echo "gem5 executable not found or not executable: $GEM5_BIN" >&2
  echo "Set GEM5_BIN=/path/to/gem5.opt and retry." >&2
  exit 1
fi

echo "========================================================="
echo "   Executing gem5 DLP Data-Level Parallelism Sweeps      "
echo "========================================================="

echo "[1/3] Running Scalar Baseline..."
"$GEM5_BIN" -d "$OUT_DIR/run1_scalar" dlp_gem5_config.py \
  --cmd=./dlp_image_bench_scalar --mode=0 --iterations="$ITERATIONS"

echo "[2/3] Running AVX SIMD Intrinsics..."
"$GEM5_BIN" -d "$OUT_DIR/run2_avx2" dlp_gem5_config.py \
  --cmd=./dlp_image_bench_avx2_gem5 --mode=1 --iterations="$ITERATIONS"

echo "[3/3] Running OpenMP SIMD..."
"$GEM5_BIN" -d "$OUT_DIR/run3_openmp" dlp_gem5_config.py \
  --cmd=./dlp_image_bench_openmp --mode=2 --iterations="$ITERATIONS"

echo "All gem5 DLP simulations completed successfully."
