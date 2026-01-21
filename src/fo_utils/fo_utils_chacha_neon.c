/**
 * @file fo_utils_chacha_neon.c
 * @brief ChaCha20 NEON-optimized FO utilities for SABER_GOST FAST configuration
 *
 * Implements Fujisaki-Okamoto transformation utilities using ChaCha20 for
 * fast deterministic coin generation.
 *
 * Key optimization (task.md 6.4.9):
 * - generate_coins uses ChaCha20 instead of iterative hashing
 * - Expected speedup: 3.3× (from task.md table 17)
 *
 * CRITICAL: Must maintain determinism!
 * Same (m, pk) → same coins (required for FO correctness)
 */

#include "../../include/fo_utils.h"
#include "../../include/hash.h"
#include "../../include/params.h"
#include "../chacha20_neon_common.h"
#include "../../external/saber_ref/fips202.h"
#include <string.h>
#include <stdlib.h>

/* ========================================================================
 * FO Utilities Implementation
 * ======================================================================== */

/* Note: chacha20_stream() is provided by chacha20_neon_common.h */

/**
 * generate_coins - Deterministic coin generation using ChaCha20
 *
 * Algorithm (from task.md 6.4.9):
 * 1. seed = SHA3-256(m || pk)  // Deterministic seed
 * 2. coins = ChaCha20(seed)     // Fast generation
 *
 * This is 3.3× faster than iterative hashing (task.md table 17).
 *
 * CRITICAL: Determinism is ESSENTIAL for FO correctness!
 * Same (m, pk) MUST produce same coins.
 */
void generate_coins(uint8_t *coins, const uint8_t *m, const uint8_t *pk)
{
    uint8_t seed[32];  /* SHA3-256 output */
    uint8_t nonce[12] = {0};  /* Fixed nonce for determinism */

    /* Step 1: Derive deterministic seed from m || pk */
    size_t total = MSG_BYTES + SABER_INDCPA_PUBLICKEYBYTES;
    uint8_t *input = malloc(total);
    if (!input) abort();

    memcpy(input, m, MSG_BYTES);
    memcpy(input + MSG_BYTES, pk, SABER_INDCPA_PUBLICKEYBYTES);

    /* seed = SHA3-256(m || pk) - DETERMINISTIC */
    sha3_256(seed, input, total);
    free(input);

    /* Step 2: Generate coins using ChaCha20 stream cipher
     *
     * ChaCha20(key=seed, nonce=0) is deterministic:
     * Same seed → same keystream → same coins
     */
    chacha20_stream(coins, SABER_NOISE_SEEDBYTES, seed, nonce, 0);

    /* Verification: coins should be identical for same (m, pk) */
}

/**
 * compute_d - Compute confirmation value d = H1(m || ct)
 *
 * No optimization needed - H1 called once per operation.
 */
void compute_d(const uint8_t *m, const uint8_t *ct, uint8_t *d)
{
    H1(d, m, MSG_BYTES, ct, SABER_BYTES_CCA_DEC);
}

/**
 * compute_shared - Compute shared key = H2(m || ct)
 *
 * No optimization needed - H2 called once per operation.
 */
void compute_shared(const uint8_t *m, const uint8_t *ct, uint8_t *key)
{
    H2(key, m, MSG_BYTES, ct, SABER_BYTES_CCA_DEC);
}
