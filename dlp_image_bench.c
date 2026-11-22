#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef DLP_AVX2
#include <immintrin.h>
#endif
#include <omp.h>

#define WIDTH 128
#define HEIGHT 128
#define SIZE (WIDTH * HEIGHT)
#define ALIGNMENT 32

// Function prototypes
void process_scalar(float *in, float *out, int iterations);
#ifdef DLP_AVX2
void process_avx2(float *in, float *out, int iterations);
#endif
void process_openmp_simd(float *in, float *out, int iterations);

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) {
    int mode = 0; // 0 = Scalar, 1 = AVX2, 2 = OpenMP SIMD
    int iterations = 100;

    if (argc > 1) mode = atoi(argv[1]);
    if (argc > 2) iterations = atoi(argv[2]);

    float *input = NULL;
    float *output = NULL;

    if (posix_memalign((void**)&input, ALIGNMENT, SIZE * sizeof(float)) != 0 ||
        posix_memalign((void**)&output, ALIGNMENT, SIZE * sizeof(float)) != 0) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    // Initialize image data with synthetic frame gradient
    for (int i = 0; i < SIZE; i++) {
        input[i] = (float)(i % 256) / 255.0f;
        output[i] = 0.0f;
    }

    printf("Starting DLP Image Processing Benchmark: Mode=%d, Iterations=%d, Grid=%dx%d\n",
           mode, iterations, WIDTH, HEIGHT);

    double start_time = get_time_sec();

    switch (mode) {
        case 0:
            process_scalar(input, output, iterations);
            break;
#ifdef DLP_AVX2
        case 1:
            process_avx2(input, output, iterations);
            break;
#endif
        case 2:
            process_openmp_simd(input, output, iterations);
            break;
        default:
            fprintf(stderr, "Invalid mode selected.\n");
            free(input);
            free(output);
            return 1;
    }

    double end_time = get_time_sec();
    double elapsed = end_time - start_time;

    printf("Execution Finished. Elapsed Time: %.6f seconds\n", elapsed);
    printf("Sample Output [0..3]: %.4f, %.4f, %.4f, %.4f\n", 
           output[0], output[1], output[2], output[3]);

    free(input);
    free(output);
    return 0;
}

// Mode 0: Scalar Baseline Loop
void process_scalar(float *in, float *out, int iterations) {
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 1; i < HEIGHT - 1; i++) {
            for (int j = 1; j < WIDTH - 1; j++) {
                int idx = i * WIDTH + j;
                // Tone mapping curve + 3x3 spatial filter kernel
                float val = in[idx];
                float tone = val / (1.0f + val); // Simple Reinhard tone mapping
                
                float blur = (in[idx - WIDTH] + in[idx + WIDTH] + 
                              in[idx - 1]     + in[idx + 1] + val) * 0.2f;
                
                out[idx] = tone * 0.5f + blur * 0.5f;
            }
        }
    }
}

// Mode 1: AVX2 256-bit SIMD Intrinsics Vector Kernel
#ifdef DLP_AVX2
#pragma GCC optimize ("no-tree-vectorize")
void process_avx2(float *in, float *out, int iterations) {
    volatile float ones_values[8];
    volatile float half_values[8];
    volatile float fifth_values[8];
    for (int i = 0; i < 8; i++) {
        ones_values[i] = 1.0f;
        half_values[i] = 0.5f;
        fifth_values[i] = 0.2f;
    }
    __m256 v_ones  = _mm256_loadu_ps((const float *)ones_values);
    __m256 v_half  = _mm256_loadu_ps((const float *)half_values);
    __m256 v_fifth = _mm256_loadu_ps((const float *)fifth_values);

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 1; i < HEIGHT - 1; i++) {
            int j = 1;
            // Vector loop processing 8 floats simultaneously
            for (; j <= WIDTH - 9; j += 8) {
                int idx = i * WIDTH + j;

                __m256 v_curr  = _mm256_loadu_ps(&in[idx]);
                __m256 v_up    = _mm256_loadu_ps(&in[idx - WIDTH]);
                __m256 v_down  = _mm256_loadu_ps(&in[idx + WIDTH]);
                __m256 v_left  = _mm256_loadu_ps(&in[idx - 1]);
                __m256 v_right = _mm256_loadu_ps(&in[idx + 1]);

                // Tone mapping: val / (1.0 + val)
                __m256 v_denom = _mm256_add_ps(v_ones, v_curr);
                __m256 v_tone  = _mm256_div_ps(v_curr, v_denom);

                // 5-point spatial blur: (up + down + left + right + curr) * 0.2
                __m256 v_sum1 = _mm256_add_ps(v_up, v_down);
                __m256 v_sum2 = _mm256_add_ps(v_left, v_right);
                __m256 v_sum  = _mm256_add_ps(v_sum1, v_sum2);
                v_sum         = _mm256_add_ps(v_sum, v_curr);
                __m256 v_blur = _mm256_mul_ps(v_sum, v_fifth);

                // Combine: tone * 0.5 + blur * 0.5
                __m256 v_t_half = _mm256_mul_ps(v_tone, v_half);
                __m256 v_b_half = _mm256_mul_ps(v_blur, v_half);
                __m256 v_res    = _mm256_add_ps(v_t_half, v_b_half);

                _mm256_storeu_ps(&out[idx], v_res);
            }

            // Scalar tail loop for remaining boundary elements
            for (; j < WIDTH - 1; j++) {
                int idx = i * WIDTH + j;
                float val = in[idx];
                float tone = val / (1.0f + val);
                float blur = (in[idx - WIDTH] + in[idx + WIDTH] + 
                              in[idx - 1]     + in[idx + 1] + val) * 0.2f;
                out[idx] = tone * 0.5f + blur * 0.5f;
            }
        }
    }
}
#endif

// Mode 2: OpenMP SIMD Compiler-Directed Loop Vectorization
void process_openmp_simd(float *in, float *out, int iterations) {
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 1; i < HEIGHT - 1; i++) {
            #pragma omp simd aligned(in, out : 32)
            for (int j = 1; j < WIDTH - 1; j++) {
                int idx = i * WIDTH + j;
                float val = in[idx];
                float tone = val / (1.0f + val);
                float blur = (in[idx - WIDTH] + in[idx + WIDTH] + 
                              in[idx - 1]     + in[idx + 1] + val) * 0.2f;
                out[idx] = tone * 0.5f + blur * 0.5f;
            }
        }
    }
}