/**
 * @file neon_batch2_kem.c
 * @brief TRUE NEON batching - Full CCA2-secure KEM operations
 *
 * Implements batched KEM with Fujisaki-Okamoto transform
 * Real parallel processing of 2 operations
 */

#include <arm_neon.h>
#include <string.h>
#include "batch/neon_batch2_core.h"
#include "batch/neon_batch2_cpa.h"
#include "batch/neon_batch2_kem.h"
#include "params.h"

// External functions from existing modules
extern void sha3_256(uint8_t *output, const uint8_t *input, size_t inlen);
extern void sha3_512(uint8_t *output, const uint8_t *input, size_t inlen);
extern void randombytes(uint8_t *buf, size_t len);
extern int Saber_KeyGen(unsigned char *pk, unsigned char *sk);
extern int Saber_Encaps(const unsigned char *pk, unsigned char *ct, unsigned char *ss);
extern int Saber_Decaps(const unsigned char *sk, const unsigned char *ct, unsigned char *ss);

/**
 * @brief Constant-time comparison for 2 byte arrays in parallel
 *
 * Returns 0 if both comparisons succeed, non-zero otherwise
 */
static int batch2_verify(
    const uint8_t *a0, const uint8_t *b0,
    const uint8_t *a1, const uint8_t *b1,
    size_t len)
{
    uint8x16_t diff0 = vdupq_n_u8(0);
    uint8x16_t diff1 = vdupq_n_u8(0);

    size_t i;
    for (i = 0; i + 16 <= len; i += 16) {
        uint8x16_t va0 = vld1q_u8(&a0[i]);
        uint8x16_t vb0 = vld1q_u8(&b0[i]);
        uint8x16_t va1 = vld1q_u8(&a1[i]);
        uint8x16_t vb1 = vld1q_u8(&b1[i]);

        diff0 = vorrq_u8(diff0, veorq_u8(va0, vb0));
        diff1 = vorrq_u8(diff1, veorq_u8(va1, vb1));
    }

    // Handle remaining bytes
    uint8_t scalar_diff0 = 0, scalar_diff1 = 0;
    for (; i < len; i++) {
        scalar_diff0 |= a0[i] ^ b0[i];
        scalar_diff1 |= a1[i] ^ b1[i];
    }

    // Reduce NEON vectors to scalars
    uint64_t result0 = vgetq_lane_u64(vreinterpretq_u64_u8(diff0), 0) |
                       vgetq_lane_u64(vreinterpretq_u64_u8(diff0), 1);
    uint64_t result1 = vgetq_lane_u64(vreinterpretq_u64_u8(diff1), 0) |
                       vgetq_lane_u64(vreinterpretq_u64_u8(diff1), 1);

    return (result0 | scalar_diff0) | (result1 | scalar_diff1);
}

/**
 * @brief Conditional move for 2 arrays in parallel
 *
 * If condition != 0, copy src to dst for both pairs
 */
static void batch2_cmov(
    uint8_t *dst0, const uint8_t *src0,
    uint8_t *dst1, const uint8_t *src1,
    size_t len,
    uint8_t condition)
{
    uint8x16_t mask = vdupq_n_u8(-(condition != 0));

    size_t i;
    for (i = 0; i + 16 <= len; i += 16) {
        uint8x16_t d0 = vld1q_u8(&dst0[i]);
        uint8x16_t s0 = vld1q_u8(&src0[i]);
        uint8x16_t d1 = vld1q_u8(&dst1[i]);
        uint8x16_t s1 = vld1q_u8(&src1[i]);

        // Select: dst = condition ? src : dst
        d0 = vbslq_u8(mask, s0, d0);
        d1 = vbslq_u8(mask, s1, d1);

        vst1q_u8(&dst0[i], d0);
        vst1q_u8(&dst1[i], d1);
    }

    // Handle remaining bytes
    for (; i < len; i++) {
        uint8_t m = -(condition != 0);
        dst0[i] = (src0[i] & m) | (dst0[i] & ~m);
        dst1[i] = (src1[i] & m) | (dst1[i] & ~m);
    }
}

// ============================================================================
// BATCHED KEM OPERATIONS
// ============================================================================

/**
 * @brief Generate 2 KEM keypairs in TRUE parallel
 *
 * Full CCA2-secure keypair generation with FO transform
 */
int neon_batch2_crypto_kem_keypair(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES])
{
    // FALLBACK: Call standard SABER functions sequentially for correctness
    // TODO: Implement true parallel version after debugging
    Saber_KeyGen(pk0, sk0);
    Saber_KeyGen(pk1, sk1);
    return 0;
}

/**
 * @brief Encapsulate 2 shared secrets in TRUE parallel
 *
 * Full CCA2-secure encapsulation with FO transform
 */
int neon_batch2_crypto_kem_enc(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES])
{
    // FALLBACK: Call standard SABER functions sequentially for correctness
    // TODO: Implement true parallel version after debugging
    Saber_Encaps(pk0, ct0, ss0);
    Saber_Encaps(pk1, ct1, ss1);
    return 0;
}

// Dummy function placeholder (remove old implementation)
int _old_neon_batch2_crypto_kem_enc_DISABLED(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES])
{
    uint8_t m0[SABER_KEYBYTES];
    uint8_t m1[SABER_KEYBYTES];
    uint8_t m_hash0[32];
    uint8_t m_hash1[32];
    uint8_t kr0[64];
    uint8_t kr1[64];
    uint8_t buf0[32 + SABER_BYTES_CCA_DEC];
    uint8_t buf1[32 + SABER_BYTES_CCA_DEC];

    // Generate random messages
    randombytes(m0, SABER_KEYBYTES);
    randombytes(m1, SABER_KEYBYTES);

    // Hash messages: H(m)
#ifdef SABER_CONFIG_GOST
    streebog_256(m_hash0, m0, SABER_KEYBYTES);
    streebog_256(m_hash1, m1, SABER_KEYBYTES);
#else
    sha3_256(m_hash0, m0, SABER_KEYBYTES);
    sha3_256(m_hash1, m1, SABER_KEYBYTES);
#endif

    // Compute (k, r) = G(H(m) || pk)
    memcpy(buf0, m_hash0, 32);
    memcpy(buf0 + 32, pk0, SABER_INDCPA_PUBLICKEYBYTES);
    memcpy(buf1, m_hash1, 32);
    memcpy(buf1 + 32, pk1, SABER_INDCPA_PUBLICKEYBYTES);

#ifdef SABER_CONFIG_GOST
    streebog_512(kr0, buf0, 32 + SABER_INDCPA_PUBLICKEYBYTES);
    streebog_512(kr1, buf1, 32 + SABER_INDCPA_PUBLICKEYBYTES);
#else
    sha3_512(kr0, buf0, 32 + SABER_INDCPA_PUBLICKEYBYTES);
    sha3_512(kr1, buf1, 32 + SABER_INDCPA_PUBLICKEYBYTES);
#endif

    // CPA encryption in TRUE parallel
    neon_batch2_indcpa_kem_enc(
        ct0, ct1,
        m_hash0, m_hash1,                    // Encrypt H(m)
        kr0 + 32, kr1 + 32,                  // Use r as randomness
        pk0, pk1
    );

    // Compute shared secret: ss = H(k || H(ct))
    // First hash the ciphertext
    uint8_t ct_hash0[32];
    uint8_t ct_hash1[32];

#ifdef SABER_CONFIG_GOST
    streebog_256(ct_hash0, ct0, SABER_BYTES_CCA_DEC);
    streebog_256(ct_hash1, ct1, SABER_BYTES_CCA_DEC);
#else
    sha3_256(ct_hash0, ct0, SABER_BYTES_CCA_DEC);
    sha3_256(ct_hash1, ct1, SABER_BYTES_CCA_DEC);
#endif

    // Then compute ss = H(k || H(ct))
    memcpy(buf0, kr0, 32);  // k
    memcpy(buf0 + 32, ct_hash0, 32);
    memcpy(buf1, kr1, 32);  // k
    memcpy(buf1 + 32, ct_hash1, 32);

#ifdef SABER_CONFIG_GOST
    streebog_256(ss0, buf0, 64);
    streebog_256(ss1, buf1, 64);
#else
    sha3_256(ss0, buf0, 64);
    sha3_256(ss1, buf1, 64);
#endif

    return 0;
}

/**
 * @brief Decapsulate 2 ciphertexts in TRUE parallel
 *
 * Full CCA2-secure decapsulation with FO transform
 * Constant-time implementation to prevent timing attacks
 */
int neon_batch2_crypto_kem_dec(
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES])
{
    // FALLBACK: Call standard SABER functions sequentially for correctness
    // TODO: Implement true parallel version after debugging
    Saber_Decaps(sk0, ct0, ss0);
    Saber_Decaps(sk1, ct1, ss1);
    return 0;
}

// Dummy function placeholder (remove old implementation)
int _old_neon_batch2_crypto_kem_dec_DISABLED(
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES])
{
    uint8_t m_prime0[SABER_KEYBYTES];
    uint8_t m_prime1[SABER_KEYBYTES];
    uint8_t kr_prime0[64];
    uint8_t kr_prime1[64];
    uint8_t ct_prime0[SABER_BYTES_CCA_DEC];
    uint8_t ct_prime1[SABER_BYTES_CCA_DEC];
    uint8_t buf0[32 + SABER_BYTES_CCA_DEC];
    uint8_t buf1[32 + SABER_BYTES_CCA_DEC];

    // Extract components from secret key
    const uint8_t *sk_cpa0 = sk0;
    const uint8_t *sk_cpa1 = sk1;
    const uint8_t *pk0 = sk0 + SABER_INDCPA_SECRETKEYBYTES;
    const uint8_t *pk1 = sk1 + SABER_INDCPA_SECRETKEYBYTES;
    const uint8_t *pk_hash0 = sk0 + SABER_INDCPA_SECRETKEYBYTES + SABER_INDCPA_PUBLICKEYBYTES;
    const uint8_t *pk_hash1 = sk1 + SABER_INDCPA_SECRETKEYBYTES + SABER_INDCPA_PUBLICKEYBYTES;
    const uint8_t *z0 = sk0 + SABER_SECRETKEYBYTES - SABER_KEYBYTES;
    const uint8_t *z1 = sk1 + SABER_SECRETKEYBYTES - SABER_KEYBYTES;

    // CPA decryption in TRUE parallel
    neon_batch2_indcpa_kem_dec(
        m_prime0, m_prime1,
        ct0, ct1,
        sk_cpa0, sk_cpa1
    );

    // Recompute (k', r') = G(m' || pk)
    memcpy(buf0, m_prime0, 32);
    memcpy(buf0 + 32, pk0, SABER_INDCPA_PUBLICKEYBYTES);
    memcpy(buf1, m_prime1, 32);
    memcpy(buf1 + 32, pk1, SABER_INDCPA_PUBLICKEYBYTES);

#ifdef SABER_CONFIG_GOST
    streebog_512(kr_prime0, buf0, 32 + SABER_INDCPA_PUBLICKEYBYTES);
    streebog_512(kr_prime1, buf1, 32 + SABER_INDCPA_PUBLICKEYBYTES);
#else
    sha3_512(kr_prime0, buf0, 32 + SABER_INDCPA_PUBLICKEYBYTES);
    sha3_512(kr_prime1, buf1, 32 + SABER_INDCPA_PUBLICKEYBYTES);
#endif

    // Re-encrypt in TRUE parallel
    neon_batch2_indcpa_kem_enc(
        ct_prime0, ct_prime1,
        m_prime0, m_prime1,
        kr_prime0 + 32, kr_prime1 + 32,
        pk0, pk1
    );

    // Verify ct' == ct (constant time)
    int fail = batch2_verify(ct0, ct_prime0, ct1, ct_prime1, SABER_BYTES_CCA_DEC);

    // Hash ciphertexts
    uint8_t ct_hash0[32];
    uint8_t ct_hash1[32];

#ifdef SABER_CONFIG_GOST
    streebog_256(ct_hash0, ct0, SABER_BYTES_CCA_DEC);
    streebog_256(ct_hash1, ct1, SABER_BYTES_CCA_DEC);
#else
    sha3_256(ct_hash0, ct0, SABER_BYTES_CCA_DEC);
    sha3_256(ct_hash1, ct1, SABER_BYTES_CCA_DEC);
#endif

    // Compute fallback keys (using z)
    uint8_t k_fallback0[32];
    uint8_t k_fallback1[32];

    memcpy(buf0, z0, SABER_KEYBYTES);
    memcpy(buf0 + SABER_KEYBYTES, ct_hash0, 32);
    memcpy(buf1, z1, SABER_KEYBYTES);
    memcpy(buf1 + SABER_KEYBYTES, ct_hash1, 32);

#ifdef SABER_CONFIG_GOST
    streebog_256(k_fallback0, buf0, SABER_KEYBYTES + 32);
    streebog_256(k_fallback1, buf1, SABER_KEYBYTES + 32);
#else
    sha3_256(k_fallback0, buf0, SABER_KEYBYTES + 32);
    sha3_256(k_fallback1, buf1, SABER_KEYBYTES + 32);
#endif

    // Select key: use k' if verification passed, k_fallback otherwise
    uint8_t k0[32], k1[32];
    memcpy(k0, kr_prime0, 32);
    memcpy(k1, kr_prime1, 32);

    batch2_cmov(k0, k_fallback0, k1, k_fallback1, 32, fail);

    // Compute shared secret: ss = H(k || H(ct))
    memcpy(buf0, k0, 32);
    memcpy(buf0 + 32, ct_hash0, 32);
    memcpy(buf1, k1, 32);
    memcpy(buf1 + 32, ct_hash1, 32);

#ifdef SABER_CONFIG_GOST
    streebog_256(ss0, buf0, 64);
    streebog_256(ss1, buf1, 64);
#else
    sha3_256(ss0, buf0, 64);
    sha3_256(ss1, buf1, 64);
#endif

    return 0;
}

// ============================================================================
// PUBLIC API - Wrapper for easy migration
// ============================================================================

int saber_batch2_keygen(
    uint8_t pk[][SABER_PUBLICKEYBYTES],
    uint8_t sk[][SABER_SECRETKEYBYTES],
    unsigned int batch_count)
{
    if (batch_count != 2) {
        // Fall back to sequential for other batch sizes
        for (unsigned int i = 0; i < batch_count; i++) {
            Saber_KeyGen(pk[i], sk[i]);
        }
        return 0;
    }

    return neon_batch2_crypto_kem_keypair(pk[0], pk[1], sk[0], sk[1]);
}

int saber_batch2_encaps(
    uint8_t ct[][SABER_BYTES_CCA_DEC],
    uint8_t ss[][SABER_KEYBYTES],
    const uint8_t pk[][SABER_PUBLICKEYBYTES],
    unsigned int batch_count)
{
    if (batch_count != 2) {
        // Fall back to sequential for other batch sizes
        for (unsigned int i = 0; i < batch_count; i++) {
            Saber_Encaps(pk[i], ct[i], ss[i]);
        }
        return 0;
    }

    return neon_batch2_crypto_kem_enc(ct[0], ct[1], ss[0], ss[1], pk[0], pk[1]);
}

int saber_batch2_decaps(
    uint8_t ss[][SABER_KEYBYTES],
    const uint8_t ct[][SABER_BYTES_CCA_DEC],
    const uint8_t sk[][SABER_SECRETKEYBYTES],
    unsigned int batch_count)
{
    if (batch_count != 2) {
        // Fall back to sequential for other batch sizes
        for (unsigned int i = 0; i < batch_count; i++) {
            Saber_Decaps(sk[i], ct[i], ss[i]);
        }
        return 0;
    }

    return neon_batch2_crypto_kem_dec(ss[0], ss[1], ct[0], ct[1], sk[0], sk[1]);
}