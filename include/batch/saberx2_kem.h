/*
 * SaberX2 NEON - Batched SABER KEM for ARM NEON (2x batching)
 */

#ifndef SABERX2_KEM_H
#define SABERX2_KEM_H

#include <stdint.h>
#include "../../external/saber_ref/SABER_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate 2 SABER keypairs in parallel
 */
int saberx2_kem_keypair(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1);

/**
 * Encapsulate 2 keys in parallel
 */
int saberx2_kem_enc(
    unsigned char *c0, unsigned char *k0, const unsigned char *pk0,
    unsigned char *c1, unsigned char *k1, const unsigned char *pk1);

/**
 * Decapsulate 2 keys in parallel
 */
int saberx2_kem_dec(
    unsigned char *k0, const unsigned char *c0, const unsigned char *sk0,
    unsigned char *k1, const unsigned char *c1, const unsigned char *sk1);

#ifdef __cplusplus
}
#endif

#endif // SABERX2_KEM_H
