# Assignment 5: DLP Benchmark

This project benchmarks Data-Level Parallelism (DLP) in three modes:

- `0`: scalar baseline
- `1`: explicit SIMD intrinsics
- `2`: OpenMP compiler-directed SIMD

The benchmark is a standalone native Linux executable. gem5 is optional and is only needed when the user wants simulated architectural statistics instead of native execution times.

## Build and run natively

From the project directory, build separate binaries so no mode accidentally enables instructions or compiler behavior intended for another mode.

### Mode 0: scalar

```bash
gcc -O3 -march=x86-64 -msse2 -mno-avx -mno-avx2 \
  dlp_image_bench.c -o dlp_image_bench_scalar -lm

./dlp_image_bench_scalar 0 100
```

### Mode 1: explicit SIMD intrinsics

For native execution on an AVX2-capable CPU:

```bash
gcc -O3 -mavx2 -fno-tree-vectorize \
  dlp_image_bench.c -DDLP_AVX2 \
  -o dlp_image_bench_avx2 -lm

./dlp_image_bench_avx2 1 100
```

For gem5 X86 builds that do not support AVX2 `vbroadcastss` or `0F38` instructions, use AVX instead:

```bash
gcc -O3 -mavx -fno-tree-vectorize \
  dlp_image_bench.c -DDLP_AVX2 \
  -o dlp_image_bench_avx2_gem5 -lm

./dlp_image_bench_avx2_gem5 1 100
```

The `-DDLP_AVX2` option includes the intrinsics implementation and enables `case 1`. The `-mavx` or `-mavx2` option controls the instruction set allowed by GCC. `-fno-tree-vectorize` prevents GCC from adding unrelated compiler-generated SIMD instructions.

### Mode 2: OpenMP SIMD

```bash
gcc -O3 -march=x86-64 -msse2 -mno-avx -mno-avx2 -fopenmp \
  dlp_image_bench.c -o dlp_image_bench_openmp -lm

./dlp_image_bench_openmp 2 100
```

The `-fopenmp` option enables the `#pragma omp simd` loop. Using `-mno-avx` and `-mno-avx2` keeps this build in a scalar/SSE baseline for clearer comparison.

## Command-line options

Each benchmark binary accepts:

```text
./dlp_image_bench_<mode> <mode> <iterations>
```

- `<mode>` selects `0`, `1`, or `2`.
- `<iterations>` controls how many times the image kernel is executed.
- Omitting arguments defaults to mode `0` and `100` iterations.
- Mode `1` is only available in binaries built with `-DDLP_AVX2`.
- The four `[0..3]` values printed at the end are boundary pixels. They remain zero because the kernels intentionally process only the interior pixels with indices `i = 1 .. HEIGHT - 2` and `j = 1 .. WIDTH - 2`.

Examples:

```bash
./dlp_image_bench_scalar 0 10
./dlp_image_bench_avx2 1 1000
./dlp_image_bench_openmp 2 1000
```

Native execution is sufficient for elapsed-time measurements and only requires GCC and the host CPU instruction set selected at build time.

## Automation scripts

The project includes scripts for building, optionally running under gem5, and summarizing gem5 statistics:

```bash
./build_dlp_benchmarks.sh
./run_dlp_experiments.sh
```

`build_dlp_benchmarks.sh` creates `dlp_image_bench_scalar`, `dlp_image_bench_avx2_gem5`, and `dlp_image_bench_openmp`.

`run_dlp_experiments.sh` runs all three binaries directly on the host; it does not require gem5:

```bash
ITERATIONS=50 ./run_dlp_experiments.sh
```

For gem5 runs, use `run_gem5_dlp_experiments.sh`. Override its gem5 path and output location if needed:

```bash
GEM5_BIN=/path/to/gem5.opt \
OUT_DIR=results_dlp \
ITERATIONS=50 \
./run_gem5_dlp_experiments.sh
```

## Optional gem5 X86 simulation

Use gem5 only if simulated instruction, cache, or timing statistics are required. It does not provide the benchmark's native wall-clock timing.

After building the desired binary, run it through the gem5 configuration:

```bash
~/gem5/build/X86/gem5.opt dlp_gem5_config.py \
  --cpu_type=o3 \
  --width=4 \
  --cmd=./dlp_image_bench_avx2_gem5 \
  --mode=1 \
  --iterations=10
```

The gem5 driver options are:

- `--cpu_type`: `o3`, `timing`, or `minor`
- `--width`: issue width for `O3CPU`
- `--bp_type`: branch predictor type
- `--cmd`: workload executable
- `--mode`: benchmark mode passed as the first workload argument
- `--iterations`: benchmark iterations passed as the second workload argument

Before adding gem5 to an experiment, confirm that the selected gem5 X86 ISA build supports every instruction in the executable. If gem5 reports `Unrecognized/invalid instruction executed` for `0F38 / 18`, it does not support `vbroadcastss`; use the `-mavx -fno-tree-vectorize` gem5-compatible build above instead of the AVX2 native build.