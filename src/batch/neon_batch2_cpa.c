/**
 * @file neon_batch2_cpa.c
 * @brief TRUE NEON batching - CPA-KEM operations
 *
 * Implements batched IND-CPA operations for SABER
 * Real parallel processing of 2 operations
 */

#include <arm_neon.h>
#include <string.h>
#include "batch/neon_batch2_core.h"
#include "batch/neon_batch2_cpa.h"
#include "params.h"

// External functions from existing modules
extern void shake128(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen);
extern void randombytes(uint8_t *buf, size_t len);
extern void cbd(const uint8_t *buf, uint16_t *s);
extern void BS2POLq(const uint8_t *bytes, uint16_t *data);
extern void POLVECp2BS(uint8_t *bytes, const uint16_t data[SABER_L][SABER_N]);
extern void POLVECq2BS(uint8_t *bytes, const uint16_t data[SABER_L][SABER_N]);
extern void BS2POLVECp(const uint8_t *bytes, uint16_t data[SABER_L][SABER_N]);
extern void BS2POLVECq(const uint8_t *bytes, uint16_t data[SABER_L][SABER_N]);
extern void BS2POLmsg(const uint8_t *bytes, uint16_t *data);
extern void POLmsg2BS(uint8_t *bytes, const uint16_t *data);
extern void BS2POLT(const uint8_t *bytes, uint16_t *data);
extern void POLT2BS(uint8_t *bytes, const uint16_t *data);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Generate matrix A from seed (shared for both operations)
 *
 * The matrix is deterministic based on seed, so we generate once
 */
static void batch2_gen_matrix(
    uint16_t A[SABER_L][SABER_L][SABER_N],
    const uint8_t seed[SABER_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYBYTES];

    // Generate matrix using SHAKE128
    for (int i = 0; i < SABER_L; i++) {
        shake128(buf, sizeof(buf), seed, SABER_SEEDBYTES);

        for (int j = 0; j < SABER_L; j++) {
            BS2POLq(&buf[j * SABER_POLYBYTES], A[i][j]);
        }
    }
}

/**
 * @brief Generate 2 secret vectors in parallel
 *
 * Uses CBD (Centered Binomial Distribution) for secret generation
 */
static void batch2_gen_secret(
    uint16_t s0[SABER_L][SABER_N],
    uint16_t s1[SABER_L][SABER_N],
    const uint8_t seed0[SABER_NOISE_SEEDBYTES],
    const uint8_t seed1[SABER_NOISE_SEEDBYTES])
{
    uint8_t buf0[SABER_L * SABER_POLYCOINBYTES];
    uint8_t buf1[SABER_L * SABER_POLYCOINBYTES];

    // Expand seeds
    shake128(buf0, sizeof(buf0), seed0, SABER_NOISE_SEEDBYTES);
    shake128(buf1, sizeof(buf1), seed1, SABER_NOISE_SEEDBYTES);

    // Generate secret polynomials using CBD
    for (int i = 0; i < SABER_L; i++) {
        cbd(&buf0[i * SABER_POLYCOINBYTES], s0[i]);
        cbd(&buf1[i * SABER_POLYCOINBYTES], s1[i]);
    }
}

// ============================================================================
// BATCHED CPA-KEM OPERATIONS
// ============================================================================

/**
 * @brief Generate 2 CPA keypairs in TRUE parallel
 *
 * Generates (pk0, sk0) and (pk1, sk1) simultaneously
 * Matrix A is generated once and shared between both operations
 */
int neon_batch2_indcpa_kem_keypair(
    uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES],
    uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES],
    uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES],
    uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES])
{
    uint16_t A[SABER_L][SABER_L][SABER_N];
    uint16_t s0[SABER_L][SABER_N];
    uint16_t s1[SABER_L][SABER_N];
    uint16_t b0[SABER_L][SABER_N];
    uint16_t b1[SABER_L][SABER_N];

    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s0[SABER_NOISE_SEEDBYTES];
    uint8_t seed_s1[SABER_NOISE_SEEDBYTES];

    // Generate random seeds
    randombytes(seed_A, SABER_SEEDBYTES);
    randombytes(seed_s0, SABER_NOISE_SEEDBYTES);
    randombytes(seed_s1, SABER_NOISE_SEEDBYTES);

    // Generate matrix A (shared)
    batch2_gen_matrix(A, seed_A);

    // Generate secret vectors in parallel
    batch2_gen_secret(s0, s1, seed_s0, seed_s1);

    // Compute b = As + h in TRUE parallel
    // This is the KEY optimization - both multiplications happen simultaneously
    neon_batch2_matrix_vector_mul(b0, b1, A, s0, s1);

    // Add h (rounding) in parallel
    for (int i = 0; i < SABER_L; i++) {
        for (int j = 0; j < SABER_N; j += 8) {
            uint16x8_t b0_vec = vld1q_u16(&b0[i][j]);
            uint16x8_t b1_vec = vld1q_u16(&b1[i][j]);

            // Shift right by (SABER_EQ - SABER_EP) bits
            b0_vec = vshrq_n_u16(b0_vec, SABER_EQ - SABER_EP);
            b1_vec = vshrq_n_u16(b1_vec, SABER_EQ - SABER_EP);

            vst1q_u16(&b0[i][j], b0_vec);
            vst1q_u16(&b1[i][j], b1_vec);
        }
    }

    // Pack public keys
    POLVECp2BS(pk0, b0);
    POLVECp2BS(pk1, b1);

    // Append seed_A to public keys
    memcpy(pk0 + SABER_POLYVECCOMPRESSEDBYTES, seed_A, SABER_SEEDBYTES);
    memcpy(pk1 + SABER_POLYVECCOMPRESSEDBYTES, seed_A, SABER_SEEDBYTES);

    // Pack secret keys
    POLVECq2BS(sk0, s0);
    POLVECq2BS(sk1, s1);

    return 0;
}

/**
 * @brief Encrypt 2 messages in TRUE parallel
 *
 * Encrypts m0 and m1 under pk0 and pk1 respectively
 * Uses shared matrix operations where possible
 */
int neon_batch2_indcpa_kem_enc(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t m0[SABER_KEYBYTES],
    const uint8_t m1[SABER_KEYBYTES],
    const uint8_t seed0[SABER_NOISE_SEEDBYTES],
    const uint8_t seed1[SABER_NOISE_SEEDBYTES],
    const uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES])
{
    uint16_t A0[SABER_L][SABER_L][SABER_N];
    uint16_t A1[SABER_L][SABER_L][SABER_N];
    uint16_t sp0[SABER_L][SABER_N];
    uint16_t sp1[SABER_L][SABER_N];
    uint16_t bp0[SABER_L][SABER_N];
    uint16_t bp1[SABER_L][SABER_N];
    uint16_t b0[SABER_L][SABER_N];
    uint16_t b1[SABER_L][SABER_N];
    uint16_t vp0[SABER_N];
    uint16_t vp1[SABER_N];
    uint16_t mp0[SABER_N];
    uint16_t mp1[SABER_N];

    const uint8_t *seed_A0 = pk0 + SABER_POLYVECCOMPRESSEDBYTES;
    const uint8_t *seed_A1 = pk1 + SABER_POLYVECCOMPRESSEDBYTES;

    // Unpack public keys
    BS2POLVECp(pk0, b0);
    BS2POLVECp(pk1, b1);

    // Check if matrices are the same (optimization opportunity)
    int same_matrix = (memcmp(seed_A0, seed_A1, SABER_SEEDBYTES) == 0);

    if (same_matrix) {
        // Generate matrix once
        batch2_gen_matrix(A0, seed_A0);
        // Point A1 to A0 (conceptually)
        memcpy(A1, A0, sizeof(A0));
    } else {
        // Generate both matrices
        batch2_gen_matrix(A0, seed_A0);
        batch2_gen_matrix(A1, seed_A1);
    }

    // Generate ephemeral secrets in parallel
    batch2_gen_secret(sp0, sp1, seed0, seed1);

    // Compute bp = A^T s' in TRUE parallel
    // Transpose multiplication: bp[i] = sum_j A[j][i] * sp[j]
    memset(bp0, 0, sizeof(bp0));
    memset(bp1, 0, sizeof(bp1));

    for (int i = 0; i < SABER_L; i++) {
        for (int j = 0; j < SABER_L; j++) {
            uint16_t temp0[2 * SABER_N];
            uint16_t temp1[2 * SABER_N];

            // Parallel multiplication
            neon_batch2_poly_mul_schoolbook(
                temp0, temp1,
                A0[j][i], A1[j][i],  // Transposed access
                sp0[j], sp1[j],
                SABER_N
            );

            // Parallel accumulation
            for (int k = 0; k < SABER_N; k += 8) {
                uint16x8_t acc0 = vld1q_u16(&bp0[i][k]);
                uint16x8_t acc1 = vld1q_u16(&bp1[i][k]);

                uint16x8_t prod0 = vld1q_u16(&temp0[k]);
                uint16x8_t prod1 = vld1q_u16(&temp1[k]);

                // Handle wraparound
                if (k + SABER_N < 2 * SABER_N) {
                    uint16x8_t wrap0 = vld1q_u16(&temp0[k + SABER_N]);
                    uint16x8_t wrap1 = vld1q_u16(&temp1[k + SABER_N]);
                    prod0 = vaddq_u16(prod0, wrap0);
                    prod1 = vaddq_u16(prod1, wrap1);
                }

                acc0 = vaddq_u16(acc0, prod0);
                acc1 = vaddq_u16(acc1, prod1);

                vst1q_u16(&bp0[i][k], acc0);
                vst1q_u16(&bp1[i][k], acc1);
            }
        }
    }

    // Round and compress bp
    for (int i = 0; i < SABER_L; i++) {
        for (int j = 0; j < SABER_N; j += 8) {
            uint16x8_t bp0_vec = vld1q_u16(&bp0[i][j]);
            uint16x8_t bp1_vec = vld1q_u16(&bp1[i][j]);

            bp0_vec = vshrq_n_u16(bp0_vec, SABER_EQ - SABER_EP);
            bp1_vec = vshrq_n_u16(bp1_vec, SABER_EQ - SABER_EP);

            vst1q_u16(&bp0[i][j], bp0_vec);
            vst1q_u16(&bp1[i][j], bp1_vec);
        }
    }

    // Compute v' = b^T s' + m in TRUE parallel
    neon_batch2_inner_product(vp0, vp1, b0, b1, sp0, sp1);

    // Scale and add message
    BS2POLmsg(m0, mp0);
    BS2POLmsg(m1, mp1);

    for (int i = 0; i < SABER_N; i += 8) {
        uint16x8_t v0 = vld1q_u16(&vp0[i]);
        uint16x8_t v1 = vld1q_u16(&vp1[i]);
        uint16x8_t m0_vec = vld1q_u16(&mp0[i]);
        uint16x8_t m1_vec = vld1q_u16(&mp1[i]);

        // Scale down
        v0 = vshrq_n_u16(v0, SABER_EP - SABER_ET);
        v1 = vshrq_n_u16(v1, SABER_EP - SABER_ET);

        // Add message
        v0 = vaddq_u16(v0, m0_vec);
        v1 = vaddq_u16(v1, m1_vec);

        // Mask to SABER_ET bits
        uint16x8_t mask = vdupq_n_u16((1 << SABER_ET) - 1);
        v0 = vandq_u16(v0, mask);
        v1 = vandq_u16(v1, mask);

        vst1q_u16(&vp0[i], v0);
        vst1q_u16(&vp1[i], v1);
    }

    // Pack ciphertexts
    POLVECp2BS(ct0, bp0);
    POLT2BS(ct0 + SABER_POLYVECCOMPRESSEDBYTES, vp0);

    POLVECp2BS(ct1, bp1);
    POLT2BS(ct1 + SABER_POLYVECCOMPRESSEDBYTES, vp1);

    return 0;
}

/**
 * @brief Decrypt 2 ciphertexts in TRUE parallel
 *
 * Decrypts ct0 and ct1 using sk0 and sk1 respectively
 */
int neon_batch2_indcpa_kem_dec(
    uint8_t m0[SABER_KEYBYTES],
    uint8_t m1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES],
    const uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES])
{
    uint16_t s0[SABER_L][SABER_N];
    uint16_t s1[SABER_L][SABER_N];
    uint16_t b0[SABER_L][SABER_N];
    uint16_t b1[SABER_L][SABER_N];
    uint16_t v0[SABER_N];
    uint16_t v1[SABER_N];
    uint16_t cm0[SABER_N];
    uint16_t cm1[SABER_N];

    // Unpack secret keys
    BS2POLVECq(sk0, s0);
    BS2POLVECq(sk1, s1);

    // Unpack ciphertext polynomials
    BS2POLVECp(ct0, b0);
    BS2POLVECp(ct1, b1);

    BS2POLT(ct0 + SABER_POLYVECCOMPRESSEDBYTES, cm0);
    BS2POLT(ct1 + SABER_POLYVECCOMPRESSEDBYTES, cm1);

    // Compute v = b^T s in TRUE parallel
    neon_batch2_inner_product(v0, v1, b0, b1, s0, s1);

    // Recover message in parallel
    for (int i = 0; i < SABER_N; i += 8) {
        uint16x8_t v0_vec = vld1q_u16(&v0[i]);
        uint16x8_t v1_vec = vld1q_u16(&v1[i]);
        uint16x8_t cm0_vec = vld1q_u16(&cm0[i]);
        uint16x8_t cm1_vec = vld1q_u16(&cm1[i]);

        // Scale down v
        v0_vec = vshrq_n_u16(v0_vec, SABER_EP - SABER_ET);
        v1_vec = vshrq_n_u16(v1_vec, SABER_EP - SABER_ET);

        // Mask to SABER_ET bits
        uint16x8_t mask = vdupq_n_u16((1 << SABER_ET) - 1);
        v0_vec = vandq_u16(v0_vec, mask);
        v1_vec = vandq_u16(v1_vec, mask);

        // Compute message = cm - v
        uint16x8_t m0_vec = vsubq_u16(cm0_vec, v0_vec);
        uint16x8_t m1_vec = vsubq_u16(cm1_vec, v1_vec);

        // Round to 1 bit
        uint16x8_t half = vdupq_n_u16(1 << (SABER_ET - 1));
        m0_vec = vaddq_u16(m0_vec, half);
        m1_vec = vaddq_u16(m1_vec, half);

        m0_vec = vshrq_n_u16(m0_vec, SABER_ET - 1);
        m1_vec = vshrq_n_u16(m1_vec, SABER_ET - 1);

        m0_vec = vandq_u16(m0_vec, vdupq_n_u16(1));
        m1_vec = vandq_u16(m1_vec, vdupq_n_u16(1));

        vst1q_u16(&v0[i], m0_vec);
        vst1q_u16(&v1[i], m1_vec);
    }

    // Pack messages
    POLmsg2BS(m0, v0);
    POLmsg2BS(m1, v1);

    return 0;
}