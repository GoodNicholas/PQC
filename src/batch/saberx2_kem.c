/*
 * SaberX2 NEON - Batched SABER KEM for ARM NEON (2x batching)
 * Adapted from SaberX4 AVX2 implementation by Sujoy Sinha Roy
 * https://github.com/sujoyetc/SaberX4
 *
 * Ported to ARM NEON for 2x batching
 */

#include <arm_neon.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "../../external/saber_ref/SABER_params.h"
#include "../../external/saber_ref/SABER_indcpa.h"
#include "../../external/saber_ref/rng.h"
#include "../../include/batch/saberx2_kem.h"
#include "saberx2_indcpa.h"
#include "../../include/batch/fips202x2_neon.h"

// Forward declarations
void indcpa_kem_keypair_x2(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1);

void indcpa_kem_enc_x2(
    unsigned char *m0, unsigned char *m1,
    unsigned char *seed_sp0, unsigned char *seed_sp1,
    unsigned char *pk0, unsigned char *c0,
    unsigned char *pk1, unsigned char *c1);

void indcpa_kem_dec_x2(
    unsigned char *sk0, unsigned char *c0, unsigned char *m0,
    unsigned char *sk1, unsigned char *c1, unsigned char *m1);

// Constant-time compare (from verify.c)
static int verify_cmp(const unsigned char *a, const unsigned char *b, size_t len)
{
    uint64_t r = 0;
    for (size_t i = 0; i < len; i++)
        r |= a[i] ^ b[i];
    return (int)r;
}

// Conditional move (from verify.c)
static void cmov(unsigned char *r, const unsigned char *x, size_t len, unsigned char b)
{
    b = -b;
    for (size_t i = 0; i < len; i++)
        r[i] ^= b & (r[i] ^ x[i]);
}

int saberx2_kem_keypair(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1)
{
    int i;

    // Generate CPA keypairs
    indcpa_kem_keypair_x2(pk0, sk0, pk1, sk1);

    // Copy pk to sk
    for (i = 0; i < SABER_INDCPA_PUBLICKEYBYTES; i++) {
        sk0[i + SABER_INDCPA_SECRETKEYBYTES] = pk0[i];
        sk1[i + SABER_INDCPA_SECRETKEYBYTES] = pk1[i];
    }

    // Hash(pk) appended to sk
    sha3_256x2(
        sk0 + SABER_SECRETKEYBYTES - 64,
        sk1 + SABER_SECRETKEYBYTES - 64,
        pk0, pk1,
        SABER_INDCPA_PUBLICKEYBYTES);

    // Random bytes for implicit rejection
    randombytes(sk0 + SABER_SECRETKEYBYTES - SABER_KEYBYTES, SABER_KEYBYTES);
    randombytes(sk1 + SABER_SECRETKEYBYTES - SABER_KEYBYTES, SABER_KEYBYTES);

    return 0;
}

int saberx2_kem_enc(
    unsigned char *c0, unsigned char *k0, const unsigned char *pk0,
    unsigned char *c1, unsigned char *k1, const unsigned char *pk1)
{
    unsigned char kr0[64], kr1[64];
    unsigned char buf0[64], buf1[64];

    // Generate random messages
    randombytes(buf0, 32);
    randombytes(buf1, 32);

    // Hash messages
    sha3_256x2(buf0, buf1, buf0, buf1, 32);

    // Hash public keys
    sha3_256x2(
        buf0 + 32, buf1 + 32,
        pk0, pk1,
        SABER_INDCPA_PUBLICKEYBYTES);

    // Derive key and randomness
    sha3_512x2(kr0, kr1, buf0, buf1, 64);

    // CPA encryption
    indcpa_kem_enc_x2(
        buf0, buf1,
        kr0 + 32, kr1 + 32,
        pk0, c0,
        pk1, c1);

    // Hash ciphertext
    sha3_256x2(
        kr0 + 32, kr1 + 32,
        c0, c1,
        SABER_BYTES_CCA_DEC);

    // Final key derivation
    sha3_256x2(k0, k1, kr0, kr1, 64);

    return 0;
}

int saberx2_kem_dec(
    unsigned char *k0, const unsigned char *c0, const unsigned char *sk0,
    unsigned char *k1, const unsigned char *c1, const unsigned char *sk1)
{
    int fail0, fail1;
    unsigned char cmp0[SABER_BYTES_CCA_DEC];
    unsigned char cmp1[SABER_BYTES_CCA_DEC];
    unsigned char buf0[64], buf1[64];
    unsigned char kr0[64], kr1[64];

    const unsigned char *pk0 = sk0 + SABER_INDCPA_SECRETKEYBYTES;
    const unsigned char *pk1 = sk1 + SABER_INDCPA_SECRETKEYBYTES;

    // CPA decryption
    indcpa_kem_dec_x2(
        sk0, c0, buf0,
        sk1, c1, buf1);

    // Copy saved hash(pk)
    for (int i = 0; i < 32; i++) {
        buf0[32 + i] = sk0[SABER_SECRETKEYBYTES - 64 + i];
        buf1[32 + i] = sk1[SABER_SECRETKEYBYTES - 64 + i];
    }

    // Derive key and randomness
    sha3_512x2(kr0, kr1, buf0, buf1, 64);

    // Re-encrypt to check
    indcpa_kem_enc_x2(
        buf0, buf1,
        kr0 + 32, kr1 + 32,
        pk0, cmp0,
        pk1, cmp1);

    // Verify ciphertexts
    fail0 = verify_cmp(c0, cmp0, SABER_BYTES_CCA_DEC);
    fail1 = verify_cmp(c1, cmp1, SABER_BYTES_CCA_DEC);

    // Hash ciphertext
    sha3_256x2(
        kr0 + 32, kr1 + 32,
        c0, c1,
        SABER_BYTES_CCA_DEC);

    // Implicit rejection
    cmov(kr0, sk0 + SABER_SECRETKEYBYTES - SABER_KEYBYTES, SABER_KEYBYTES, (unsigned char)fail0);
    cmov(kr1, sk1 + SABER_SECRETKEYBYTES - SABER_KEYBYTES, SABER_KEYBYTES, (unsigned char)fail1);

    // Final key derivation
    sha3_256x2(k0, k1, kr0, kr1, 64);

    return 0;
}

// Wrapper functions for batch2 API compatibility
int saber_batch2_keygen(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES])
{
    return saberx2_kem_keypair(pk0, sk0, pk1, sk1);
}

int saber_batch2_encaps(
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES])
{
    return saberx2_kem_enc(ct0, ss0, pk0, ct1, ss1, pk1);
}

int saber_batch2_decaps(
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES])
{
    return saberx2_kem_dec(ss0, ct0, sk0, ss1, ct1, sk1);
}
