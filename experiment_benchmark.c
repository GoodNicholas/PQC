/**
 * @file experiment_benchmark.c
 * @brief SABER GOST Performance Benchmark - Full Methodology Implementation
 *
 * Implements experimental protocol from METHODOLOGY.md:
 * - N=1000 measurements per operation
 * - Warmup=100 iterations
 * - CLOCK_MONOTONIC timer (μs precision)
 * - Sequential and batched (2x) measurements
 * - Raw data saved to .dat files for statistical analysis
 *
 * Usage: ./experiment_benchmark <output_directory>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "api.h"

#ifdef ENABLE_BATCHING
#include "batch/batch_kem.h"
#endif

// Experimental parameters (per METHODOLOGY.md section 6.2)
#define N_MEASUREMENTS 1000
#define N_WARMUP 100

/**
 * Get current time in microseconds
 * Uses CLOCK_MONOTONIC per METHODOLOGY.md section 6.1.2
 */
static inline double get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

/**
 * Save measurements array to file
 * Format: one measurement per line (μs)
 */
static int save_measurements(const char* filepath, const double* data, int count, const char* operation) {
    FILE* f = fopen(filepath, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open %s for writing\n", filepath);
        return -1;
    }

    fprintf(f, "# SABER GOST Performance Measurements\n");
    fprintf(f, "# Operation: %s\n", operation);
    fprintf(f, "# Measurements: N=%d, Warmup=%d\n", count, N_WARMUP);
    fprintf(f, "# Timer: CLOCK_MONOTONIC\n");
    fprintf(f, "# Units: microseconds (μs)\n");
    fprintf(f, "#\n");

    for (int i = 0; i < count; i++) {
        fprintf(f, "%.3f\n", data[i]);
    }

    fclose(f);
    return 0;
}

/**
 * Benchmark single operation (KeyGen, Encaps, or Decaps)
 */
static int benchmark_operation(
    const char* output_dir,
    const char* filename,
    const char* operation_name,
    void (*operation)(void),
    double* measurements
) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", output_dir, filename);

    printf("  Benchmarking %s...\n", operation_name);

    // Warmup (section 6.2.1 - M ≥ 100)
    printf("    Warmup: %d iterations...\n", N_WARMUP);
    for (int i = 0; i < N_WARMUP; i++) {
        operation();
    }

    // Main measurements (section 6.2.2 - N = 1000)
    printf("    Measurements: %d iterations...\n", N_MEASUREMENTS);
    for (int i = 0; i < N_MEASUREMENTS; i++) {
        double t_start = get_time_us();
        operation();
        double t_end = get_time_us();
        measurements[i] = t_end - t_start;

        // Progress indicator every 100 iterations
        if ((i + 1) % 100 == 0) {
            printf("      Progress: %d/%d\n", i + 1, N_MEASUREMENTS);
        }
    }

    // Save to file
    printf("    Saving to %s...\n", filename);
    if (save_measurements(filepath, measurements, N_MEASUREMENTS, operation_name) != 0) {
        return -1;
    }

    // Quick preview (mean)
    double sum = 0.0;
    for (int i = 0; i < N_MEASUREMENTS; i++) {
        sum += measurements[i];
    }
    double mean = sum / N_MEASUREMENTS;
    printf("    Preview: mean = %.2f μs (%.2f ops/sec)\n", mean, 1000000.0 / mean);

    return 0;
}

/**
 * Main benchmark routine
 */
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_directory>\n", argv[0]);
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s /root/saber_results/DEFAULT\n", argv[0]);
        return 1;
    }

    const char* output_dir = argv[1];

    // Create output directory if needed
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        if (mkdir(output_dir, 0755) != 0) {
            fprintf(stderr, "ERROR: Cannot create directory %s\n", output_dir);
            return 1;
        }
    }

    // Print banner
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   SABER GOST Performance Benchmark - Full Methodology   ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Configuration:\n");
#ifdef SABER_CONFIG_DEFAULT
    printf("  SABER_CONFIG:       DEFAULT\n");
#elif defined(SABER_CONFIG_FAST)
    printf("  SABER_CONFIG:       FAST\n");
#elif defined(SABER_CONFIG_GOST)
    printf("  SABER_CONFIG:       GOST\n");
#elif defined(SABER_CONFIG_GOST_FAST)
    printf("  SABER_CONFIG:       GOST_FAST\n");
#else
    printf("  SABER_CONFIG:       UNKNOWN\n");
#endif

#ifdef ENABLE_BATCHING
    printf("  ENABLE_BATCHING:    ON\n");
#else
    printf("  ENABLE_BATCHING:    OFF\n");
#endif

    printf("\n");
    printf("Parameters:\n");
    printf("  Public key:         %d bytes\n", SABER_PUBLIC_KEY_BYTES);
    printf("  Secret key:         %d bytes\n", SABER_SECRET_KEY_BYTES);
    printf("  Ciphertext:         %d bytes\n", SABER_CIPHERTEXT_BYTES);
    printf("  Shared secret:      %d bytes\n", SABER_SHARED_KEY_BYTES);
    printf("\n");
    printf("Methodology (METHODOLOGY.md section 6.2):\n");
    printf("  Measurements (N):   %d\n", N_MEASUREMENTS);
    printf("  Warmup (M):         %d\n", N_WARMUP);
    printf("  Timer:              CLOCK_MONOTONIC\n");
    printf("  Output directory:   %s\n", output_dir);
    printf("\n");
    printf("════════════════════════════════════════════════════════════\n\n");

    // Allocate measurement arrays
    double* measurements = (double*)malloc(N_MEASUREMENTS * sizeof(double));
    if (!measurements) {
        fprintf(stderr, "ERROR: Cannot allocate memory for measurements\n");
        return 1;
    }

    // Allocate key material
    uint8_t pk[SABER_PUBLIC_KEY_BYTES];
    uint8_t sk[SABER_SECRET_KEY_BYTES];
    uint8_t ct[SABER_CIPHERTEXT_BYTES];
    uint8_t ss_enc[SABER_SHARED_KEY_BYTES];
    uint8_t ss_dec[SABER_SHARED_KEY_BYTES];

#ifndef ENABLE_BATCHING
    // =====================================================================
    // NON-BATCHING MODE: Standard sequential measurements
    // =====================================================================
    printf("MODE: Sequential (non-batching)\n");
    printf("════════════════════════════════════════════════════════════\n\n");

    // KeyGen wrapper
    void keygen_wrapper(void) {
        Saber_KeyGen(pk, sk);
    }

    // Encaps wrapper
    void encaps_wrapper(void) {
        Saber_Encaps(pk, ct, ss_enc);
    }

    // Decaps wrapper
    void decaps_wrapper(void) {
        Saber_Decaps(sk, ct, ss_dec);
    }

    // Generate key once for Encaps/Decaps
    Saber_KeyGen(pk, sk);
    Saber_Encaps(pk, ct, ss_enc);

    // Benchmark operations
    printf("Operation 1/3: KeyGen\n");
    if (benchmark_operation(output_dir, "keygen.dat", "KeyGen", keygen_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }
    printf("\n");

    printf("Operation 2/3: Encaps\n");
    if (benchmark_operation(output_dir, "encaps.dat", "Encaps", encaps_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }
    printf("\n");

    printf("Operation 3/3: Decaps\n");
    if (benchmark_operation(output_dir, "decaps.dat", "Decaps", decaps_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }
    printf("\n");

#else // ENABLE_BATCHING
    // =====================================================================
    // BATCHING MODE: Measure sequential (2×1) vs batched (1×2)
    // =====================================================================
    printf("MODE: Batching (2x parallel)\n");
    printf("════════════════════════════════════════════════════════════\n\n");

    // Allocate batch arrays
    uint8_t pk_batch[2][SABER_PUBLIC_KEY_BYTES];
    uint8_t sk_batch[2][SABER_SECRET_KEY_BYTES];
    uint8_t ct_batch[2][SABER_CIPHERTEXT_BYTES];
    uint8_t ss_batch[2][SABER_SHARED_KEY_BYTES];

    // Initialize batching system
    printf("Initializing batching system...\n");
    if (saber_batch_init() != 0) {
        fprintf(stderr, "ERROR: Batching initialization failed (NEON not available?)\n");
        free(measurements);
        return 1;
    }
    printf("  Config: %s\n\n", saber_batch_get_config());

    // -------------------------------------------------------------------------
    // KeyGen: Sequential (2×1) vs Batched (1×2)
    // -------------------------------------------------------------------------
    printf("Operation 1/3: KeyGen\n");
    printf("──────────────────────────────────────────────────────────────\n");

    // Sequential: 2 separate KeyGen calls
    printf("  [1/2] Sequential mode (2 × single KeyGen)...\n");

    void keygen_seq_wrapper(void) {
        Saber_KeyGen(pk_batch[0], sk_batch[0]);
        Saber_KeyGen(pk_batch[1], sk_batch[1]);
    }

    if (benchmark_operation(output_dir, "keygen_seq.dat", "KeyGen Sequential (2x)",
                           keygen_seq_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }

    // Batched: 1 batched KeyGen call
    printf("  [2/2] Batched mode (1 × batch_keygen(2))...\n");

    void keygen_batch_wrapper(void) {
        saber_batch_keygen(pk_batch, sk_batch, 2);
    }

    if (benchmark_operation(output_dir, "keygen_batch.dat", "KeyGen Batched (2x)",
                           keygen_batch_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }
    printf("\n");

    // -------------------------------------------------------------------------
    // Encaps: Sequential (2×1) vs Batched (1×2)
    // -------------------------------------------------------------------------
    printf("Operation 2/3: Encaps\n");
    printf("──────────────────────────────────────────────────────────────\n");

    // Generate keys for Encaps
    saber_batch_keygen(pk_batch, sk_batch, 2);

    // Sequential: 2 separate Encaps calls
    printf("  [1/2] Sequential mode (2 × single Encaps)...\n");

    void encaps_seq_wrapper(void) {
        Saber_Encaps(pk_batch[0], ct_batch[0], ss_batch[0]);
        Saber_Encaps(pk_batch[1], ct_batch[1], ss_batch[1]);
    }

    if (benchmark_operation(output_dir, "encaps_seq.dat", "Encaps Sequential (2x)",
                           encaps_seq_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }

    // Batched: 1 batched Encaps call
    printf("  [2/2] Batched mode (1 × batch_encaps(2))...\n");

    void encaps_batch_wrapper(void) {
        saber_batch_encaps(ct_batch, ss_batch, (const uint8_t(*)[SABER_PUBLIC_KEY_BYTES])pk_batch, 2);
    }

    if (benchmark_operation(output_dir, "encaps_batch.dat", "Encaps Batched (2x)",
                           encaps_batch_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }
    printf("\n");

    // -------------------------------------------------------------------------
    // Decaps: Sequential (2×1) vs Batched (1×2)
    // -------------------------------------------------------------------------
    printf("Operation 3/3: Decaps\n");
    printf("──────────────────────────────────────────────────────────────\n");

    // Generate ciphertexts for Decaps
    saber_batch_encaps(ct_batch, ss_batch, (const uint8_t(*)[SABER_PUBLIC_KEY_BYTES])pk_batch, 2);

    // Sequential: 2 separate Decaps calls
    printf("  [1/2] Sequential mode (2 × single Decaps)...\n");

    void decaps_seq_wrapper(void) {
        Saber_Decaps(sk_batch[0], ct_batch[0], ss_batch[0]);
        Saber_Decaps(sk_batch[1], ct_batch[1], ss_batch[1]);
    }

    if (benchmark_operation(output_dir, "decaps_seq.dat", "Decaps Sequential (2x)",
                           decaps_seq_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }

    // Batched: 1 batched Decaps call
    printf("  [2/2] Batched mode (1 × batch_decaps(2))...\n");

    void decaps_batch_wrapper(void) {
        saber_batch_decaps(ss_batch,
                          (const uint8_t(*)[SABER_CIPHERTEXT_BYTES])ct_batch,
                          (const uint8_t(*)[SABER_SECRET_KEY_BYTES])sk_batch, 2);
    }

    if (benchmark_operation(output_dir, "decaps_batch.dat", "Decaps Batched (2x)",
                           decaps_batch_wrapper, measurements) != 0) {
        free(measurements);
        return 1;
    }
    printf("\n");

    // Cleanup batching
    saber_batch_cleanup();
#endif // ENABLE_BATCHING

    // Success
    printf("════════════════════════════════════════════════════════════\n");
    printf("✓ Benchmark completed successfully!\n");
    printf("\n");
    printf("Output files saved to: %s/\n", output_dir);
#ifndef ENABLE_BATCHING
    printf("  - keygen.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - encaps.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - decaps.dat (N=%d measurements)\n", N_MEASUREMENTS);
#else
    printf("  - keygen_seq.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - keygen_batch.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - encaps_seq.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - encaps_batch.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - decaps_seq.dat (N=%d measurements)\n", N_MEASUREMENTS);
    printf("  - decaps_batch.dat (N=%d measurements)\n", N_MEASUREMENTS);
#endif
    printf("\n");
    printf("Next steps:\n");
    printf("  1. Run statistical analysis: python3 analyze_results.py %s\n", output_dir);
    printf("  2. Generate visualizations: python3 visualize_results.py %s\n", output_dir);
    printf("\n");

    free(measurements);
    return 0;
}
