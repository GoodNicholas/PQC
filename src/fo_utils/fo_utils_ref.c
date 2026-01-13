/**
 * @file fo_utils_ref.c
 * @brief Reference implementation of FO-transformation utilities for SABER
 *
 * Used for DEFAULT and TEST configurations.
 * Uses SHA-3 based hash functions (H1, H2) for all operations.
 */

#include <string.h>
#include <stdint.h>
#include "../../include/fo_utils.h"
#include "../../include/hash.h"
#include "../../include/params.h"

/**
 * Generate deterministic coins for encryption
 * Algorithm: coins = H1(m || pk)
 */
void generate_coins(uint8_t *coins,
                    const uint8_t *m,
                    const uint8_t *pk) {
    // H1(m || pk) -> coins (NOISE_BYTES = 32 bytes)
    H1(coins, m, MSG_BYTES, pk, PK_BYTES);
}

/**
 * Compute confirmation hash d = H1(m || ct)
 */
void compute_d(const uint8_t *m,
               const uint8_t *ct,
               uint8_t *d) {
    // H1(m || ct) -> d (D_BYTES = 32 bytes)
    H1(d, m, MSG_BYTES, ct, CT_CORE_BYTES);
}

/**
 * Compute shared secret key = H2(m || ct)
 */
void compute_shared(const uint8_t *m,
                    const uint8_t *ct,
                    uint8_t *shared_key) {
    // H2(m || ct) -> shared_key (SABER_KEYBYTES = 32 bytes)
    H2(shared_key, m, MSG_BYTES, ct, CT_CORE_BYTES);
}
