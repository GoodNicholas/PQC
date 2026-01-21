/**
 * @file batch_core.h
 * @brief NEON-optimized batched SABER core operations interface
 */

#ifndef BATCH_CORE_H
#define BATCH_CORE_H

#include <stdint.h>
#include "core/SABER_params.h"

#define BATCH_SIZE 2

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Batched CPA key generation using NEON
 *
 * Generates 2 CPA key pairs in parallel.
 */
int batch_indcpa_kem_keypair(uint8_t pk[BATCH_SIZE][SABER_INDCPA_PUBLICKEYBYTES],
                             uint8_t sk[BATCH_SIZE][SABER_INDCPA_SECRETKEYBYTES]);

/**
 * @brief Batched CPA encryption using NEON
 *
 * Encrypts 2 messages in parallel.
 */
int batch_indcpa_kem_enc(uint8_t ct[BATCH_SIZE][SABER_BYTES_CCA_DEC],
                        const uint8_t m[BATCH_SIZE][SABER_KEYBYTES],
                        const uint8_t noiseseed[BATCH_SIZE][SABER_COINBYTES],
                        const uint8_t pk[BATCH_SIZE][SABER_INDCPA_PUBLICKEYBYTES]);

/**
 * @brief Batched CPA decryption using NEON
 *
 * Decrypts 2 ciphertexts in parallel.
 */
int batch_indcpa_kem_dec(uint8_t m[BATCH_SIZE][SABER_KEYBYTES],
                        const uint8_t sk[BATCH_SIZE][SABER_INDCPA_SECRETKEYBYTES],
                        const uint8_t ct[BATCH_SIZE][SABER_BYTES_CCA_DEC]);

#ifdef __cplusplus
}
#endif

#endif /* BATCH_CORE_H */