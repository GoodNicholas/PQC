/**
 * @file neon_batch2_kem.h
 * @brief TRUE NEON batching interface - Full KEM operations
 *
 * CCA2-secure KEM with real parallel processing
 * Production-ready implementation with ~2x throughput
 */

#ifndef NEON_BATCH2_KEM_H
#define NEON_BATCH2_KEM_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// FULL CCA2-SECURE KEM OPERATIONS
// ============================================================================

/**
 * @brief Generate 2 KEM keypairs in TRUE parallel
 *
 * Generates two complete CCA2-secure keypairs simultaneously.
 * Matrix generation is shared between operations for efficiency.
 *
 * Expected performance: ~1.8-2x speedup over sequential generation
 *
 * @param pk0, pk1 Output public keys [SABER_PUBLICKEYBYTES]
 * @param sk0, sk1 Output secret keys [SABER_SECRETKEYBYTES]
 * @return 0 on success
 */
int neon_batch2_crypto_kem_keypair(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES]
);

/**
 * @brief Encapsulate 2 shared secrets in TRUE parallel
 *
 * Creates two encapsulated shared secrets simultaneously.
 * Uses FO transform for CCA2 security.
 *
 * Expected performance: ~1.7-1.9x speedup over sequential encapsulation
 *
 * @param ct0, ct1 Output ciphertexts [SABER_BYTES_CCA_DEC]
 * @param ss0, ss1 Output shared secrets [SABER_KEYBYTES]
 * @param pk0, pk1 Public keys [SABER_PUBLICKEYBYTES]
 * @return 0 on success
 */
int neon_batch2_crypto_kem_enc(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES]
);

/**
 * @brief Decapsulate 2 ciphertexts in TRUE parallel
 *
 * Recovers two shared secrets simultaneously.
 * Constant-time implementation prevents timing attacks.
 * Uses implicit rejection for CCA2 security.
 *
 * Expected performance: ~1.6-1.8x speedup over sequential decapsulation
 *
 * @param ss0, ss1 Output shared secrets [SABER_KEYBYTES]
 * @param ct0, ct1 Ciphertexts to decrypt [SABER_BYTES_CCA_DEC]
 * @param sk0, sk1 Secret keys [SABER_SECRETKEYBYTES]
 * @return 0 on success
 */
int neon_batch2_crypto_kem_dec(
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES]
);

// ============================================================================
// PUBLIC API - COMPATIBLE WITH EXISTING INTERFACE
// ============================================================================

/**
 * @brief Batched key generation (public API)
 *
 * Automatically uses NEON batching for batch_count == 2
 * Falls back to sequential for other batch sizes
 *
 * @param pk Array of public keys [batch_count][SABER_PUBLICKEYBYTES]
 * @param sk Array of secret keys [batch_count][SABER_SECRETKEYBYTES]
 * @param batch_count Number of keypairs to generate
 * @return 0 on success
 */
int saber_batch2_keygen(
    uint8_t pk[][SABER_PUBLICKEYBYTES],
    uint8_t sk[][SABER_SECRETKEYBYTES],
    unsigned int batch_count
);

/**
 * @brief Batched encapsulation (public API)
 *
 * Automatically uses NEON batching for batch_count == 2
 * Falls back to sequential for other batch sizes
 *
 * @param ct Array of ciphertexts [batch_count][SABER_BYTES_CCA_DEC]
 * @param ss Array of shared secrets [batch_count][SABER_KEYBYTES]
 * @param pk Array of public keys [batch_count][SABER_PUBLICKEYBYTES]
 * @param batch_count Number of encapsulations
 * @return 0 on success
 */
int saber_batch2_encaps(
    uint8_t ct[][SABER_BYTES_CCA_DEC],
    uint8_t ss[][SABER_KEYBYTES],
    const uint8_t pk[][SABER_PUBLICKEYBYTES],
    unsigned int batch_count
);

/**
 * @brief Batched decapsulation (public API)
 *
 * Automatically uses NEON batching for batch_count == 2
 * Falls back to sequential for other batch sizes
 *
 * @param ss Array of shared secrets [batch_count][SABER_KEYBYTES]
 * @param ct Array of ciphertexts [batch_count][SABER_BYTES_CCA_DEC]
 * @param sk Array of secret keys [batch_count][SABER_SECRETKEYBYTES]
 * @param batch_count Number of decapsulations
 * @return 0 on success
 */
int saber_batch2_decaps(
    uint8_t ss[][SABER_KEYBYTES],
    const uint8_t ct[][SABER_BYTES_CCA_DEC],
    const uint8_t sk[][SABER_SECRETKEYBYTES],
    unsigned int batch_count
);

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

/**
 * @brief Get expected speedup for batched operations
 *
 * Returns the theoretical speedup based on operation type
 *
 * @param operation "keygen", "encaps", or "decaps"
 * @return Expected speedup factor (e.g., 1.8 for 80% improvement)
 */
float saber_batch2_get_speedup(const char *operation);

/**
 * @brief Check if NEON is available on current platform
 *
 * @return 1 if NEON is available, 0 otherwise
 */
int saber_batch2_neon_available(void);

#ifdef __cplusplus
}
#endif

#endif // NEON_BATCH2_KEM_H