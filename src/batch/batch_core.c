/**
 * @file batch_core.c
 * @brief NEON-optimized batched SABER core operations (Level 2)
 *
 * Implements parallel processing of 2 SABER operations using ARM NEON SIMD.
 * Based on SaberX4 architecture but adapted for ARM NEON (2x instead of 4x).
 */

#include <arm_neon.h>
#include <string.h>
#include <stdint.h>
#include "../external/saber_ref/SABER_params.h"
#include "batch/neon_batch_poly.h"
#include "batch/batch_core.h"

// Define missing COINBYTES as NOISE_SEEDBYTES
#define SABER_COINBYTES SABER_NOISE_SEEDBYTES
#define SABER_SEED_A_SIZE SABER_SEEDBYTES

#define BATCH_SIZE 2

// Polynomial structures
typedef struct {
    uint16_t vec[SABER_N];
} poly;

typedef struct {
    poly vec[SABER_L];
} polyvec;

// External functions
extern void shake128(uint8_t *output, size_t outlen, const uint8_t *input, size_t inlen);
extern void randombytes(uint8_t *buf, size_t len);
extern void GenSecret(polyvec s[SABER_L], const uint8_t *seed);
extern void BS2POLq(const uint8_t *bytes, poly *data);
extern void POLq2BS(uint8_t *bytes, const poly *data);
extern void BS2POLVECq(const uint8_t *bytes, polyvec *data);
extern void POLVECq2BS(uint8_t *bytes, const polyvec *data);
extern void BS2POLVECp(const uint8_t *bytes, polyvec *data);
extern void POLVECp2BS(uint8_t *bytes, const polyvec *data);
extern void BS2POLVEC(const uint8_t *bytes, polyvec *data, uint16_t modulus);
extern void POLVEC2BS(uint8_t *bytes, const polyvec *data, uint16_t modulus);
extern void BS2POLT(const uint8_t *bytes, poly *data);
extern void POLT2BS(uint8_t *bytes, const poly *data);
extern void BS2POLmsg(const uint8_t *bytes, poly *data);
extern void POLmsg2BS(uint8_t *bytes, const poly *data);

// Constants from SABER
#define h1 4
#define h2 3

// ============================================================================
// Batched Matrix Operations
// ============================================================================

/**
 * @brief Generate 2 random matrices A in parallel using NEON
 *
 * Generates seedA from seed, then uses XOF to generate matrix elements.
 * Processes 2 matrices simultaneously for better cache utilization.
 */
void batch_GenMatrix(poly A[BATCH_SIZE][SABER_L][SABER_L],
                     const uint8_t seed[BATCH_SIZE][SABER_SEEDBYTES])
{
    uint8_t seedA[BATCH_SIZE][SABER_SEED_A_SIZE];
    uint8_t buf[BATCH_SIZE][SABER_L * SABER_POLYBYTES];

    // Generate seedA for both operations in parallel
    for (int b = 0; b < BATCH_SIZE; b++) {
        shake128(seedA[b], SABER_SEED_A_SIZE, seed[b], SABER_SEEDBYTES);
    }

    // Generate matrix coefficients
    for (int i = 0; i < SABER_L; i++) {
        for (int b = 0; b < BATCH_SIZE; b++) {
            // Use XOF to expand seed
            shake128(buf[b], SABER_L * SABER_POLYBYTES, seedA[b], SABER_SEED_A_SIZE);

            for (int j = 0; j < SABER_L; j++) {
                BS2POLq(buf[b] + j * SABER_POLYBYTES, &A[b][i][j]);
            }
        }
    }
}

/**
 * @brief Batched matrix-vector multiplication using NEON
 *
 * Computes res = A * s for 2 matrix-vector pairs in parallel.
 * Uses NEON to process multiple coefficients simultaneously.
 */
void batch_MatrixVectorMul(polyvec res[BATCH_SIZE][SABER_L],
                          const poly A[BATCH_SIZE][SABER_L][SABER_L],
                          const polyvec s[BATCH_SIZE][SABER_L])
{
    // Temporary storage for intermediate results
    uint16_t acc[BATCH_SIZE][SABER_N];
    uint16_t temp[BATCH_SIZE][SABER_N];

    for (int i = 0; i < SABER_L; i++) {
        // Initialize accumulator
        memset(acc, 0, sizeof(acc));

        for (int j = 0; j < SABER_L; j++) {
            // Extract polynomials for batching
            uint16_t a_poly[BATCH_SIZE][SABER_N];
            uint16_t s_poly[BATCH_SIZE][SABER_N];

            for (int b = 0; b < BATCH_SIZE; b++) {
                memcpy(a_poly[b], A[b][i][j].vec, SABER_N * sizeof(uint16_t));
                memcpy(s_poly[b], s[b][j].vec[0].vec, SABER_N * sizeof(uint16_t));
            }

            // Batch polynomial multiplication using NEON
            batch2_poly_mul(temp, a_poly, s_poly);

            // Accumulate results using NEON
            for (int k = 0; k < SABER_N; k += 8) {
                uint16x8_t acc0 = vld1q_u16(&acc[0][k]);
                uint16x8_t acc1 = vld1q_u16(&acc[1][k]);
                uint16x8_t tmp0 = vld1q_u16(&temp[0][k]);
                uint16x8_t tmp1 = vld1q_u16(&temp[1][k]);

                acc0 = vaddq_u16(acc0, tmp0);
                acc1 = vaddq_u16(acc1, tmp1);

                vst1q_u16(&acc[0][k], acc0);
                vst1q_u16(&acc[1][k], acc1);
            }
        }

        // Store results
        for (int b = 0; b < BATCH_SIZE; b++) {
            memcpy(res[b][i].vec[0].vec, acc[b], SABER_N * sizeof(uint16_t));
        }
    }
}

/**
 * @brief Batched inner product computation using NEON
 *
 * Computes res = <b, s> for 2 pairs in parallel.
 */
void batch_InnerProd(poly res[BATCH_SIZE],
                    const polyvec b[BATCH_SIZE][SABER_L],
                    const polyvec s[BATCH_SIZE][SABER_L])
{
    uint16_t acc[BATCH_SIZE][SABER_N];
    uint16_t temp[BATCH_SIZE][SABER_N];

    // Initialize accumulator
    memset(acc, 0, sizeof(acc));

    for (int j = 0; j < SABER_L; j++) {
        // Extract polynomials
        uint16_t b_poly[BATCH_SIZE][SABER_N];
        uint16_t s_poly[BATCH_SIZE][SABER_N];

        for (int bat = 0; bat < BATCH_SIZE; bat++) {
            memcpy(b_poly[bat], b[bat][j].vec[0].vec, SABER_N * sizeof(uint16_t));
            memcpy(s_poly[bat], s[bat][j].vec[0].vec, SABER_N * sizeof(uint16_t));
        }

        // Batch multiplication
        batch2_poly_mul(temp, b_poly, s_poly);

        // Accumulate using NEON
        for (int k = 0; k < SABER_N; k += 8) {
            uint16x8_t acc0 = vld1q_u16(&acc[0][k]);
            uint16x8_t acc1 = vld1q_u16(&acc[1][k]);
            uint16x8_t tmp0 = vld1q_u16(&temp[0][k]);
            uint16x8_t tmp1 = vld1q_u16(&temp[1][k]);

            acc0 = vaddq_u16(acc0, tmp0);
            acc1 = vaddq_u16(acc1, tmp1);

            vst1q_u16(&acc[0][k], acc0);
            vst1q_u16(&acc[1][k], acc1);
        }
    }

    // Store results
    for (int bat = 0; bat < BATCH_SIZE; bat++) {
        memcpy(res[bat].vec, acc[bat], SABER_N * sizeof(uint16_t));
    }
}

// ============================================================================
// Batched CPA Encryption Core
// ============================================================================

/**
 * @brief Batched CPA key generation using NEON
 *
 * Generates 2 CPA key pairs in parallel.
 */
int batch_indcpa_kem_keypair(uint8_t pk[BATCH_SIZE][SABER_INDCPA_PUBLICKEYBYTES],
                             uint8_t sk[BATCH_SIZE][SABER_INDCPA_SECRETKEYBYTES])
{
    poly A[BATCH_SIZE][SABER_L][SABER_L];
    polyvec s[BATCH_SIZE][SABER_L];
    polyvec b[BATCH_SIZE][SABER_L];

    uint8_t seed[BATCH_SIZE][SABER_SEEDBYTES];
    uint8_t noiseseed[BATCH_SIZE][SABER_COINBYTES];

    // Generate random seeds for both operations
    for (int i = 0; i < BATCH_SIZE; i++) {
        randombytes(seed[i], SABER_SEEDBYTES);
        shake128(noiseseed[i], SABER_COINBYTES, seed[i], SABER_SEEDBYTES);

        // Pack seed in public key
        memcpy(pk[i], seed[i], SABER_SEEDBYTES);
    }

    // Generate matrix A in parallel
    batch_GenMatrix(A, seed);

    // Generate secret vectors in parallel
    for (int i = 0; i < BATCH_SIZE; i++) {
        GenSecret(s[i], noiseseed[i]);
    }

    // Compute b = A^T * s in parallel
    poly A_T[BATCH_SIZE][SABER_L][SABER_L];

    // Transpose matrices
    for (int bat = 0; bat < BATCH_SIZE; bat++) {
        for (int i = 0; i < SABER_L; i++) {
            for (int j = 0; j < SABER_L; j++) {
                A_T[bat][j][i] = A[bat][i][j];
            }
        }
    }

    // Matrix-vector multiplication in parallel
    batch_MatrixVectorMul(b, A_T, s);

    // Round and pack
    for (int bat = 0; bat < BATCH_SIZE; bat++) {
        // Round b
        for (int i = 0; i < SABER_L; i++) {
            for (int j = 0; j < SABER_N; j++) {
                b[bat][i].vec[0].vec[j] = (b[bat][i].vec[0].vec[j] + h1) >> (SABER_EQ - SABER_EP);
            }
        }

        // Pack b in public key
        POLVECp2BS(pk[bat] + SABER_SEEDBYTES, &b[bat][0]);

        // Pack s in secret key
        POLVECq2BS(sk[bat], &s[bat][0]);
    }

    return 0;
}

/**
 * @brief Batched CPA encryption using NEON
 *
 * Encrypts 2 messages in parallel.
 */
int batch_indcpa_kem_enc(uint8_t ct[BATCH_SIZE][SABER_BYTES_CCA_DEC],
                        const uint8_t m[BATCH_SIZE][SABER_KEYBYTES],
                        const uint8_t noiseseed[BATCH_SIZE][SABER_COINBYTES],
                        const uint8_t pk[BATCH_SIZE][SABER_INDCPA_PUBLICKEYBYTES])
{
    poly A[BATCH_SIZE][SABER_L][SABER_L];
    polyvec s_prime[BATCH_SIZE][SABER_L];
    polyvec b[BATCH_SIZE][SABER_L];
    polyvec b_prime[BATCH_SIZE][SABER_L];
    poly v_prime[BATCH_SIZE];
    poly m_poly[BATCH_SIZE];

    uint8_t seed[BATCH_SIZE][SABER_SEEDBYTES];

    // Extract seed from public keys
    for (int i = 0; i < BATCH_SIZE; i++) {
        memcpy(seed[i], pk[i], SABER_SEEDBYTES);
    }

    // Generate matrix A in parallel
    batch_GenMatrix(A, seed);

    // Generate s' in parallel
    for (int i = 0; i < BATCH_SIZE; i++) {
        GenSecret(s_prime[i], noiseseed[i]);
    }

    // Compute b' = A * s' in parallel
    batch_MatrixVectorMul(b_prime, A, s_prime);

    // Unpack b from public key and compute v' in parallel
    for (int i = 0; i < BATCH_SIZE; i++) {
        BS2POLVECp(pk[i] + SABER_SEEDBYTES, &b[i][0]);
    }

    // Compute v' = <b, s'> in parallel
    batch_InnerProd(v_prime, b, s_prime);

    // Process message and add
    for (int bat = 0; bat < BATCH_SIZE; bat++) {
        // Convert message to polynomial
        BS2POLmsg(m[bat], &m_poly[bat]);

        // Add message (using NEON for vectorization)
        for (int j = 0; j < SABER_N; j += 8) {
            uint16x8_t v = vld1q_u16(&v_prime[bat].vec[j]);
            uint16x8_t msg = vld1q_u16(&m_poly[bat].vec[j]);
            uint16x8_t h1_vec = vdupq_n_u16(h1);

            v = vaddq_u16(v, h1_vec);
            v = vshrq_n_u16(v, SABER_EP - 1);
            v = vaddq_u16(v, msg);
            v = vandq_u16(v, vdupq_n_u16(SABER_P - 1));

            vst1q_u16(&v_prime[bat].vec[j], v);
        }

        // Pack ciphertext
        POLVEC2BS(ct[bat], &b_prime[bat][0], SABER_EQ - SABER_EP);
        POLT2BS(ct[bat] + SABER_POLYVECCOMPRESSEDBYTES, &v_prime[bat]);
    }

    return 0;
}

/**
 * @brief Batched CPA decryption using NEON
 *
 * Decrypts 2 ciphertexts in parallel.
 */
int batch_indcpa_kem_dec(uint8_t m[BATCH_SIZE][SABER_KEYBYTES],
                        const uint8_t sk[BATCH_SIZE][SABER_INDCPA_SECRETKEYBYTES],
                        const uint8_t ct[BATCH_SIZE][SABER_BYTES_CCA_DEC])
{
    polyvec s[BATCH_SIZE][SABER_L];
    polyvec b_prime[BATCH_SIZE][SABER_L];
    poly v[BATCH_SIZE];
    poly v_result[BATCH_SIZE];

    // Unpack secret key and ciphertext in parallel
    for (int i = 0; i < BATCH_SIZE; i++) {
        BS2POLVECq(sk[i], &s[i][0]);
        BS2POLVEC(ct[i], &b_prime[i][0], SABER_EQ - SABER_EP);
        BS2POLT(ct[i] + SABER_POLYVECCOMPRESSEDBYTES, &v[i]);
    }

    // Compute v_result = v - <b', s> in parallel
    batch_InnerProd(v_result, b_prime, s);

    // Decrypt messages using NEON
    for (int bat = 0; bat < BATCH_SIZE; bat++) {
        for (int j = 0; j < SABER_N; j += 8) {
            uint16x8_t v_vec = vld1q_u16(&v[bat].vec[j]);
            uint16x8_t vr_vec = vld1q_u16(&v_result[bat].vec[j]);
            uint16x8_t h2_vec = vdupq_n_u16(h2);

            // v = v - v_result + h2
            v_vec = vsubq_u16(v_vec, vr_vec);
            v_vec = vaddq_u16(v_vec, h2_vec);
            v_vec = vshrq_n_u16(v_vec, SABER_EP - 1);
            v_vec = vandq_u16(v_vec, vdupq_n_u16(0x01));

            vst1q_u16(&v[bat].vec[j], v_vec);
        }

        // Convert to message bytes
        POLmsg2BS(m[bat], &v[bat]);
    }

    return 0;
}

// ============================================================================
// Helper Functions for Polynomial Operations
// ============================================================================

/**
 * @brief Batched polynomial multiplication wrapper
 *
 * Multiplies 2 pairs of polynomials in parallel using NEON.
 */
void batch2_poly_mul(uint16_t res[BATCH_SIZE][SABER_N],
                    const uint16_t a[BATCH_SIZE][SABER_N],
                    const uint16_t b[BATCH_SIZE][SABER_N])
{
    // Use Toom-Cook 4-way for 256-coefficient polynomials
    // Note: batch2_toom4_neon signature expects separate arrays
    uint16_t c0[2 * SABER_N];
    uint16_t c1[2 * SABER_N];

    batch2_toom4_neon(c0, c1, a[0], a[1], b[0], b[1]);

    // Copy results back (only first SABER_N coefficients)
    memcpy(res[0], c0, SABER_N * sizeof(uint16_t));
    memcpy(res[1], c1, SABER_N * sizeof(uint16_t));
}