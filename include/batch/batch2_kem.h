/**
 * @file batch2_kem.h
 * @brief REAL 2x batching for SABER KEM operations
 *
 * Based on SaberX4 architecture adapted for ARM NEON.
 * Provides production-ready batched KEM operations without placeholders.
 */

#ifndef SABER_BATCH2_KEM_H
#define SABER_BATCH2_KEM_H

#include <stdint.h>
#include "../params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Batched key generation for 2 keypairs
 *
 * TRUE BATCHING: Matrix operations use batch2_matrix_vector_mul,
 * which processes both keypairs with interleaved NEON.
 *
 * @param pk0, pk1 Output: public keys
 * @param sk0, sk1 Output: secret keys
 * @return 0 on success
 */
int saber_batch2_keygen(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES]
);

/**
 * @brief Batched encapsulation for 2 messages
 *
 * TRUE BATCHING: Shared matrix A loaded once, used for both operations
 * with batch2_matrix_vector_mul.
 *
 * @param ct0, ct1 Output: ciphertexts
 * @param ss0, ss1 Output: shared secrets
 * @param pk0, pk1 Input: public keys
 * @return 0 on success
 */
int saber_batch2_encaps(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES]
);

/**
 * @brief Batched decapsulation for 2 ciphertexts
 *
 * TRUE BATCHING: Inner product computation uses batch2_inner_product.
 *
 * @param ss0, ss1 Output: shared secrets
 * @param ct0, ct1 Input: ciphertexts
 * @param sk0, sk1 Input: secret keys
 * @return 0 on success
 */
int saber_batch2_decaps(
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES]
);

#ifdef __cplusplus
}
#endif

#endif // SABER_BATCH2_KEM_H
