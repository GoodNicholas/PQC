/**
 * @file batch2_kem_simple.c
 * @brief SIMPLE 2x batched SABER KEM - sequential fallback
 *
 * This is a WORKING fallback that calls standard SABER functions twice.
 * Not TRUE parallelism, but correct and working.
 */

#include "batch/batch2_kem.h"
#include "params.h"
#include <string.h>

// External SABER functions
extern int Saber_KeyGen(uint8_t *pk, uint8_t *sk);
extern int Saber_Encaps(const uint8_t *pk, uint8_t *ct, uint8_t *ss);
extern int Saber_Decaps(const uint8_t *sk, const uint8_t *ct, uint8_t *ss);

int saber_batch2_keygen(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES])
{
    Saber_KeyGen(pk0, sk0);
    Saber_KeyGen(pk1, sk1);
    return 0;
}

int saber_batch2_encaps(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES])
{
    Saber_Encaps(pk0, ct0, ss0);
    Saber_Encaps(pk1, ct1, ss1);
    return 0;
}

int saber_batch2_decaps(
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES])
{
    Saber_Decaps(sk0, ct0, ss0);
    Saber_Decaps(sk1, ct1, ss1);
    return 0;
}
