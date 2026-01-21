#ifndef INDCPA2X_H
#define INDCPA2X_H

#include <stdint.h>
#include "SABER_params.h"

/*
 * SaberX2 Parallel IND-CPA Functions
 * Processes 2 KEM instances simultaneously using NEON parallelization
 */

void indcpa_kem_keypair2x(
    uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES], uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES],
    uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES], uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES],
    const uint8_t seed_A0[SABER_SEEDBYTES], const uint8_t seed_A1[SABER_SEEDBYTES],
    const uint8_t seed_s0[SABER_NOISE_SEEDBYTES], const uint8_t seed_s1[SABER_NOISE_SEEDBYTES]);

void indcpa_kem_enc2x(
    const uint8_t m0[SABER_KEYBYTES], const uint8_t seed_sp0[SABER_NOISE_SEEDBYTES],
    const uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES], uint8_t ciphertext0[SABER_BYTES_CCA_DEC],
    const uint8_t m1[SABER_KEYBYTES], const uint8_t seed_sp1[SABER_NOISE_SEEDBYTES],
    const uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES], uint8_t ciphertext1[SABER_BYTES_CCA_DEC]);

void indcpa_kem_dec2x(
    const uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES], const uint8_t ciphertext0[SABER_BYTES_CCA_DEC], uint8_t m0[SABER_KEYBYTES],
    const uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES], const uint8_t ciphertext1[SABER_BYTES_CCA_DEC], uint8_t m1[SABER_KEYBYTES]);

#endif // INDCPA2X_H
