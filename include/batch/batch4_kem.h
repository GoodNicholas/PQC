/**
 * @file batch4_kem.h
 * @brief SaberX4 NEON - 4-way parallel KEM operations
 */

#ifndef BATCH4_KEM_H
#define BATCH4_KEM_H

#include <stdint.h>
#include <stddef.h>
#include "params.h"

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

/**
 * @brief Initialize SaberX4 batching system
 * @return 0 on success, negative on error
 */
int saber_batch4_init(void);

/**
 * @brief Clean up SaberX4 batching system
 */
void saber_batch4_cleanup(void);

/**
 * @brief Get configuration string for SaberX4
 * @return Configuration description
 */
const char* saber_batch4_get_config(void);

/**
 * @brief Generate 4 keypairs in parallel
 * @param pk Array of public keys [4][SABER_PUBLICKEYBYTES]
 * @param sk Array of secret keys [4][SABER_SECRETKEYBYTES]
 * @return 0 on success
 */
int saber_batch4_keygen(
    uint8_t pk[][SABER_PUBLICKEYBYTES],
    uint8_t sk[][SABER_SECRETKEYBYTES]
);

/**
 * @brief Encapsulate 4 shared secrets in parallel
 * @param ct Array of ciphertexts [4][SABER_CIPHERTEXTBYTES]
 * @param ss Array of shared secrets [4][SABER_SHAREDSECRETBYTES]
 * @param pk Array of public keys [4][SABER_PUBLICKEYBYTES]
 * @return 0 on success
 */
int saber_batch4_encaps(
    uint8_t ct[][SABER_CIPHERTEXTBYTES],
    uint8_t ss[][SABER_SHAREDSECRETBYTES],
    const uint8_t pk[][SABER_PUBLICKEYBYTES]
);

/**
 * @brief Decapsulate 4 shared secrets in parallel
 * @param ss Array of shared secrets [4][SABER_SHAREDSECRETBYTES]
 * @param ct Array of ciphertexts [4][SABER_CIPHERTEXTBYTES]
 * @param sk Array of secret keys [4][SABER_SECRETKEYBYTES]
 * @return 0 on success
 */
int saber_batch4_decaps(
    uint8_t ss[][SABER_SHAREDSECRETBYTES],
    const uint8_t ct[][SABER_CIPHERTEXTBYTES],
    const uint8_t sk[][SABER_SECRETKEYBYTES]
);

// Wrapper functions for generic batch API compatibility
int saber_batch_init(void);
void saber_batch_cleanup(void);
const char* saber_batch_get_config(void);
int saber_batch_keygen(uint8_t pk[][SABER_PUBLICKEYBYTES],
                      uint8_t sk[][SABER_SECRETKEYBYTES],
                      unsigned int batch_count);
int saber_batch_encaps(uint8_t ct[][SABER_CIPHERTEXTBYTES],
                      uint8_t ss[][SABER_SHAREDSECRETBYTES],
                      const uint8_t pk[][SABER_PUBLICKEYBYTES],
                      unsigned int batch_count);
int saber_batch_decaps(uint8_t ss[][SABER_SHAREDSECRETBYTES],
                      const uint8_t ct[][SABER_CIPHERTEXTBYTES],
                      const uint8_t sk[][SABER_SECRETKEYBYTES],
                      unsigned int batch_count);

#endif // BATCH4_KEM_H