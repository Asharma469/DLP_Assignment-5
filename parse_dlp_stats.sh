#!/bin/bash
set -euo pipefail

RESULTS_DIR="${RESULTS_DIR:-results_dlp}"

printf "========================================================================================================\n"
printf "%-20s | %-10s | %-16s | %-16s | %-14s\n" "Mode Configuration" "IPC" "simTicks" "simInsts" "Speedup (x)"
printf "========================================================================================================\n"

baseline_ticks=""

for run in run1_scalar run2_avx2 run3_openmp; do
  file="$RESULTS_DIR/$run/stats.txt"
  if [[ -f "$file" ]]; then
    ipc=$(awk '/^system\.cpu\.ipc / {print $2; exit}' "$file")
    ticks=$(awk '/^simTicks / {print $2; exit}' "$file")
    insts=$(awk '/^simInsts / {print $2; exit}' "$file")

    if [[ "$run" == "run1_scalar" ]]; then
      baseline_ticks="$ticks"
      speedup="1.00x"
    elif [[ -n "$baseline_ticks" && -n "$ticks" && "$ticks" != "0" ]]; then
      speedup=$(awk -v base="$baseline_ticks" -v ticks="$ticks" 'BEGIN {printf "%.2fx", base / ticks}')
    else
      speedup="N/A"
    fi

    printf "%-20s | %-10s | %-16s | %-16s | %-14s\n" \
      "$run" "${ipc:-N/A}" "${ticks:-N/A}" "${insts:-N/A}" "$speedup"
  fi
done

printf "========================================================================================================\n"
