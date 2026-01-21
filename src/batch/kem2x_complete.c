#include <string.h>
#include <stdint.h>
#include "kem.h"
#include "SABER_params.h"
#include "randombytes.h"
#include "fips202.h"
#include "fips202x2.h"
#include "indcpa2x.h"

// Parallel SHA3 function declarations
extern void sha3_256x2(uint8_t *out0, uint8_t *out1,
                      const uint8_t *in0, const uint8_t *in1, size_t inlen);
extern void sha3_512x2(uint8_t *out0, uint8_t *out1,
                      const uint8_t *in0, const uint8_t *in1, size_t inlen);

/*
 * SaberX2 Complete Implementation
 * Processes 2 KEM operations in parallel using NEON
 */

int crypto_kem_keypair2x(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1)
{
    int i;

    unsigned char seed_A0[SABER_SEEDBYTES];
    unsigned char seed_A1[SABER_SEEDBYTES];

    unsigned char seed_s0[SABER_NOISE_SEEDBYTES];
    unsigned char seed_s1[SABER_NOISE_SEEDBYTES];

    unsigned char pseudorandom_key0[SABER_KEYBYTES];
    unsigned char pseudorandom_key1[SABER_KEYBYTES];

    // Generate random seeds for both instances
    randombytes(seed_A0, SABER_SEEDBYTES);
    randombytes(seed_s0, SABER_NOISE_SEEDBYTES);
    randombytes(pseudorandom_key0, SABER_KEYBYTES);

    randombytes(seed_A1, SABER_SEEDBYTES);
    randombytes(seed_s1, SABER_NOISE_SEEDBYTES);
    randombytes(pseudorandom_key1, SABER_KEYBYTES);

    // Hash seeds (not revealing system RNG state)
    shake128(seed_A0, SABER_SEEDBYTES, seed_A0, SABER_SEEDBYTES);
    shake128(seed_A1, SABER_SEEDBYTES, seed_A1, SABER_SEEDBYTES);

    // Parallel key generation using indcpa2x
    indcpa_kem_keypair2x(pk0, sk0, pk1, sk1,
                         seed_A0, seed_A1,
                         seed_s0, seed_s1);

    // Append pseudorandom keys to secret keys
    for(i = 0; i < SABER_KEYBYTES; i++) {
        sk0[SABER_INDCPA_SECRETKEYBYTES + i] = pseudorandom_key0[i];
        sk1[SABER_INDCPA_SECRETKEYBYTES + i] = pseudorandom_key1[i];
    }

    // Parallel SHA3-256 hash of public keys
    sha3_256x2(sk0 + SABER_SECRETKEYBYTES - 64,
               sk1 + SABER_SECRETKEYBYTES - 64,
               pk0, pk1,
               SABER_INDCPA_PUBLICKEYBYTES);

    // Copy public keys to secret keys
    for(i = 0; i < SABER_INDCPA_PUBLICKEYBYTES; i++) {
        sk0[i + SABER_INDCPA_SECRETKEYBYTES + SABER_KEYBYTES] = pk0[i];
        sk1[i + SABER_INDCPA_SECRETKEYBYTES + SABER_KEYBYTES] = pk1[i];
    }

    return 0;
}

int crypto_kem_enc2x(
    unsigned char *ct0, unsigned char *ss0, const unsigned char *pk0,
    unsigned char *ct1, unsigned char *ss1, const unsigned char *pk1)
{
    unsigned char kr0[64];
    unsigned char kr1[64];

    unsigned char buf0[32 + SABER_INDCPA_PUBLICKEYBYTES];
    unsigned char buf1[32 + SABER_INDCPA_PUBLICKEYBYTES];

    unsigned char m0[SABER_KEYBYTES];
    unsigned char m1[SABER_KEYBYTES];

    // Generate random messages
    randombytes(m0, SABER_KEYBYTES);
    randombytes(m1, SABER_KEYBYTES);

    // SHA3-256 hash of messages
    sha3_256(buf0, m0, SABER_KEYBYTES);
    sha3_256(buf1, m1, SABER_KEYBYTES);

    // SHA3-512 to generate (r, K) for both instances
    // Parallel SHA3-512
    memcpy(buf0 + 32, pk0, SABER_INDCPA_PUBLICKEYBYTES);
    memcpy(buf1 + 32, pk1, SABER_INDCPA_PUBLICKEYBYTES);

    sha3_512x2(kr0, kr1,
               buf0, buf1,
               SABER_INDCPA_PUBLICKEYBYTES + 32);

    // Parallel encapsulation using indcpa2x
    indcpa_kem_enc2x(m0, kr0, pk0, ct0,
                     m1, kr1, pk1, ct1);

    // Parallel SHA3-256 hash of key part to derive shared secrets
    sha3_256x2(ss0, ss1,
               kr0 + 32, kr1 + 32,
               32);

    return 0;
}

int crypto_kem_dec2x(
    unsigned char *ss0, const unsigned char *ct0, const unsigned char *sk0,
    unsigned char *ss1, const unsigned char *ct1, const unsigned char *sk1)
{
    int i;
    unsigned char fail0, fail1;

    unsigned char cmp0[SABER_BYTES_CCA_DEC];
    unsigned char cmp1[SABER_BYTES_CCA_DEC];

    unsigned char buf0[32 + SABER_INDCPA_PUBLICKEYBYTES];
    unsigned char buf1[32 + SABER_INDCPA_PUBLICKEYBYTES];

    unsigned char kr0[64];
    unsigned char kr1[64];

    unsigned char m_dec0[SABER_KEYBYTES];
    unsigned char m_dec1[SABER_KEYBYTES];

    const unsigned char *pk0 = sk0 + SABER_INDCPA_SECRETKEYBYTES + SABER_KEYBYTES;
    const unsigned char *pk1 = sk1 + SABER_INDCPA_SECRETKEYBYTES + SABER_KEYBYTES;

    // Parallel decapsulation using indcpa2x
    indcpa_kem_dec2x(sk0, ct0, m_dec0,
                     sk1, ct1, m_dec1);

    // SHA3-256 hash of decrypted messages
    sha3_256(buf0, m_dec0, SABER_KEYBYTES);
    sha3_256(buf1, m_dec1, SABER_KEYBYTES);

    // SHA3-512 to re-derive (r, K)
    memcpy(buf0 + 32, pk0, SABER_INDCPA_PUBLICKEYBYTES);
    memcpy(buf1 + 32, pk1, SABER_INDCPA_PUBLICKEYBYTES);

    sha3_512x2(kr0, kr1,
               buf0, buf1,
               SABER_INDCPA_PUBLICKEYBYTES + 32);

    // Re-encapsulate to verify
    indcpa_kem_enc2x(m_dec0, kr0, pk0, cmp0,
                     m_dec1, kr1, pk1, cmp1);

    // Constant-time comparison
    fail0 = 0;
    fail1 = 0;

    for(i = 0; i < SABER_BYTES_CCA_DEC; i++) {
        fail0 |= ct0[i] ^ cmp0[i];
        fail1 |= ct1[i] ^ cmp1[i];
    }

    // If verification fails, use pseudorandom key instead
    // Overwrite kr+32 with either re-derived key or pseudorandom key
    for(i = 0; i < SABER_KEYBYTES; i++) {
        kr0[i + 32] = (kr0[i + 32] & (unsigned char)(-(1 - ((uint16_t)fail0 >> 8)))) |
                      (sk0[SABER_INDCPA_SECRETKEYBYTES + i] & (unsigned char)(-((uint16_t)fail0 >> 8)));

        kr1[i + 32] = (kr1[i + 32] & (unsigned char)(-(1 - ((uint16_t)fail1 >> 8)))) |
                      (sk1[SABER_INDCPA_SECRETKEYBYTES + i] & (unsigned char)(-((uint16_t)fail1 >> 8)));
    }

    // Compute shared secret (use 32-byte key from kr+32)
    sha3_256x2(ss0, ss1,
               kr0 + 32, kr1 + 32,
               32);

    return 0;
}
