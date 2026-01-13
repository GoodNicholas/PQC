/**
 * @file core_neon_ntt_wrapper.c
 * @brief Thin wrapper for neon-ntt SABER_indcpa.c to match SABER_GOST API
 *
 * This file provides thin wrappers around neon-ntt's indcpa functions
 * to match the SABER_GOST core.h API signatures.
 */

#include "../../include/core.h"
#include "../../include/rng.h"

// Forward declarations from neon-ntt SABER_indcpa.c
extern void indcpa_kem_keypair(uint8_t pk[], uint8_t sk[]);
extern void indcpa_kem_enc(const uint8_t m[], const uint8_t seed_sp[],
                            const uint8_t pk[], uint8_t ciphertext[]);
extern void indcpa_kem_dec(const uint8_t sk[], const uint8_t ciphertext[],
                            uint8_t m[]);

// Note: randombytes is provided by rng_system.c, no need to re-define it here

/**
 * SaberCore_KeyGen - wrapper для indcpa_kem_keypair
 */
void SaberCore_KeyGen(uint8_t *pk, uint8_t *sk)
{
    indcpa_kem_keypair(pk, sk);
}

/**
 * SaberCore_Encrypt - wrapper для indcpa_kem_enc
 * API: SaberCore_Encrypt(pk, m, coins, ct)
 * neon-ntt: indcpa_kem_enc(m, seed_sp, pk, ct)
 */
void SaberCore_Encrypt(const uint8_t *pk, const uint8_t *m,
                       const uint8_t *coins, uint8_t *ct)
{
    indcpa_kem_enc(m, coins, pk, ct);
}

/**
 * SaberCore_Decrypt - wrapper для indcpa_kem_dec
 */
void SaberCore_Decrypt(const uint8_t *sk, const uint8_t *ct, uint8_t *m)
{
    indcpa_kem_dec(sk, ct, m);
}
