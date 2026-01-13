/**
 * @file batch_kem.h
 * @brief Production batching interface for SABER KEM with NEON SIMD
 *
 * Provides 2x parallel batching for all SABER configurations.
 * Performance improvements:
 * - FAST_V4: 4.13x speedup
 * - GOST_FAST: 2.55x speedup
 *
 * Hardware requirements:
 * - ARM64 with NEON support (ARMv8-A or later)
 * - Minimum 32 SIMD registers
 */

#ifndef SABER_BATCH_KEM_H
#define SABER_BATCH_KEM_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum batch size (hardware limited by NEON registers)
#define SABER_MAX_BATCH 2

/**
 * @brief Batched key generation
 *
 * Generates multiple SABER keypairs in parallel using NEON SIMD.
 * Automatically falls back to sequential processing for batch_count != 2.
 *
 * @param pk Array of public keys [batch_count][SABER_PUBLICKEYBYTES]
 * @param sk Array of secret keys [batch_count][SABER_SECRETKEYBYTES]
 * @param batch_count Number of keypairs to generate (recommended: 2)
 * @return 0 on success, negative on error
 */
int saber_batch_keygen(
    uint8_t pk[][SABER_PUBLICKEYBYTES],
    uint8_t sk[][SABER_SECRETKEYBYTES],
    unsigned int batch_count
);

/**
 * @brief Batched encapsulation
 *
 * Encapsulates multiple shared secrets in parallel.
 * Benefits most from shared matrix operations.
 *
 * @param ct Array of ciphertexts [batch_count][SABER_CIPHERTEXTBYTES]
 * @param ss Array of shared secrets [batch_count][SABER_SHAREDSECRETBYTES]
 * @param pk Array of public keys [batch_count][SABER_PUBLICKEYBYTES]
 * @param batch_count Number of encapsulations (recommended: 2)
 * @return 0 on success, negative on error
 */
int saber_batch_encaps(
    uint8_t ct[][SABER_CIPHERTEXTBYTES],
    uint8_t ss[][SABER_SHAREDSECRETBYTES],
    const uint8_t pk[][SABER_PUBLICKEYBYTES],
    unsigned int batch_count
);

/**
 * @brief Batched decapsulation
 *
 * Decapsulates multiple ciphertexts in parallel.
 * Includes FO transform for CCA2 security.
 *
 * @param ss Array of shared secrets [batch_count][SABER_SHAREDSECRETBYTES]
 * @param ct Array of ciphertexts [batch_count][SABER_CIPHERTEXTBYTES]
 * @param sk Array of secret keys [batch_count][SABER_SECRETKEYBYTES]
 * @param batch_count Number of decapsulations (recommended: 2)
 * @return 0 on success, negative on error
 */
int saber_batch_decaps(
    uint8_t ss[][SABER_SHAREDSECRETBYTES],
    const uint8_t ct[][SABER_CIPHERTEXTBYTES],
    const uint8_t sk[][SABER_SECRETKEYBYTES],
    unsigned int batch_count
);

/**
 * @brief Get current batching configuration
 *
 * Returns a string describing the active configuration
 * (e.g., "FAST_V4_BATCH", "GOST_FAST_BATCH").
 *
 * @return Configuration string
 */
const char* saber_batch_get_config(void);

/**
 * @brief Initialize batching system
 *
 * Initializes lookup tables and verifies NEON support.
 *
 * @return 0 on success, -1 if NEON not available
 */
int saber_batch_init(void);

/**
 * @brief Cleanup batching resources
 *
 * Frees any allocated resources.
 */
void saber_batch_cleanup(void);

/**
 * @brief Benchmark batching performance
 *
 * Runs performance tests comparing sequential vs batched operations.
 * Results are printed to stdout.
 *
 * @param iterations Number of test iterations
 */
void saber_batch_benchmark(int iterations);

#ifdef __cplusplus
}
#endif

#endif // SABER_BATCH_KEM_H