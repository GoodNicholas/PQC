/**
 * @file neon_batch_poly.h
 * @brief ARM NEON batched polynomial operations for SABER FAST implementation
 *
 * This implements batched polynomial multiplication using NEON SIMD instructions,
 * allowing parallel processing of multiple polynomials.
 *
 * Based on:
 * - PQC_NEON implementations by GMUCERG
 * - "Neon NTT: Faster Dilithium, Kyber, and Saber on Cortex-A72 and Apple M1"
 * - AVX2 batching from SaberX4
 */

#ifndef NEON_BATCH_POLY_H
#define NEON_BATCH_POLY_H

#include <stdint.h>
#include <arm_neon.h>

// SABER parameters
#define SABER_N 256
#define SABER_Q 8192
#define SABER_EQ 13
#define SABER_EP 10
#define SABER_ET 4

// Batch sizes - process 2 or 4 polynomials in parallel
#define BATCH2 2
#define BATCH4 4

// Toom-Cook parameters
#define TC_BLOCK 64  // 256/4 = 64 coefficients per block
#define TC_BLOCK_RES (2*TC_BLOCK)

// Karatsuba parameters
#define KARA_BLOCK 16  // 64/4 = 16 coefficients per block
#define KARA_BLOCK_RES (2*KARA_BLOCK)

/**
 * @brief Batched polynomial multiplication using Toom-Cook 4-way
 *
 * Multiplies 2 polynomial pairs in parallel using NEON SIMD
 *
 * @param c0,c1 Output polynomials (2*SABER_N coefficients each)
 * @param a0,a1 First input polynomials (SABER_N coefficients each)
 * @param b0,b1 Second input polynomials (SABER_N coefficients each)
 */
void batch2_toom4_neon(
    uint16_t c0[2*SABER_N], uint16_t c1[2*SABER_N],
    const uint16_t a0[SABER_N], const uint16_t a1[SABER_N],
    const uint16_t b0[SABER_N], const uint16_t b1[SABER_N]
);

/**
 * @brief Batched polynomial multiplication using Toom-Cook 4-way
 *
 * Multiplies 4 polynomial pairs in parallel using NEON SIMD
 *
 * @param c Output polynomials array (4 polynomials, 2*SABER_N coefficients each)
 * @param a First input polynomials array (4 polynomials, SABER_N coefficients each)
 * @param b Second input polynomials array (4 polynomials, SABER_N coefficients each)
 */
void batch4_toom4_neon(
    uint16_t c[4][2*SABER_N],
    const uint16_t a[4][SABER_N],
    const uint16_t b[4][SABER_N]
);

/**
 * @brief Batched schoolbook multiplication for small polynomials
 *
 * Multiplies multiple 16-coefficient polynomial pairs in parallel
 * Used as base case for recursive algorithms
 *
 * @param c Output polynomials (transposed format)
 * @param a First input polynomials (transposed format)
 * @param b Second input polynomials (transposed format)
 * @param batch_count Number of polynomial pairs to multiply
 */
void batch_schoolbook16_neon(
    uint16_t *restrict c,
    const uint16_t *restrict a,
    const uint16_t *restrict b,
    unsigned int batch_count
);

/**
 * @brief Matrix transpose for batched operations
 *
 * Transposes polynomial coefficients to enable SIMD batching
 * Converts from AoS (Array of Structures) to SoA (Structure of Arrays)
 *
 * @param output Transposed output (SoA format)
 * @param input Original input (AoS format)
 * @param rows Number of polynomials
 * @param cols Number of coefficients per polynomial
 */
void transpose_poly_batch(
    uint16_t *restrict output,
    const uint16_t *restrict input,
    unsigned int rows,
    unsigned int cols
);

/**
 * @brief Inverse matrix transpose after batched operations
 *
 * Converts back from SoA to AoS format
 *
 * @param output Original format output (AoS)
 * @param input Transposed input (SoA)
 * @param rows Number of polynomials
 * @param cols Number of coefficients per polynomial
 */
void inverse_transpose_poly_batch(
    uint16_t *restrict output,
    const uint16_t *restrict input,
    unsigned int rows,
    unsigned int cols
);

// Helper macros for NEON operations

// Load 8 uint16 values
#define neon_load(c, a) c = vld1q_u16(a)

// Store 8 uint16 values
#define neon_store(a, c) vst1q_u16(a, c)

// Multiply: c = a * b
#define neon_mul(c, a, b) c = vmulq_u16(a, b)

// Multiply-accumulate: c += a * b
#define neon_mla(c, a, b) c = vmlaq_u16(c, a, b)

// Add: c = a + b
#define neon_add(c, a, b) c = vaddq_u16(a, b)

// Subtract: c = a - b
#define neon_sub(c, a, b) c = vsubq_u16(a, b)

// Shift left: c = a << n
#define neon_shl(c, a, n) c = vshlq_n_u16(a, n)

// Shift right: c = a >> n
#define neon_shr(c, a, n) c = vshrq_n_u16(a, n)

/**
 * @brief Batched modular reduction
 *
 * Reduces multiple values modulo SABER_Q in parallel
 *
 * @param values NEON vector of values to reduce
 * @return Reduced values
 */
static inline uint16x8_t batch_reduce_mod_q(uint16x8_t values) {
    const uint16x8_t q = vdupq_n_u16(SABER_Q);
    const uint16x8_t q_minus_1 = vdupq_n_u16(SABER_Q - 1);

    // Barrett reduction for SABER_Q = 8192 = 2^13
    // Since Q is a power of 2, we can use bitwise AND
    return vandq_u16(values, q_minus_1);
}

/**
 * @brief Batched Barrett multiplication
 *
 * Computes (a * b * inv) mod q for multiple values in parallel
 * Optimized for SABER parameters
 *
 * @param a,b Input vectors
 * @param inv Precomputed inverse for Barrett reduction
 * @return Result vector
 */
static inline uint16x8_t batch_barrett_mul(uint16x8_t a, uint16x8_t b, uint16_t inv) {
    // Multiply
    uint32x4_t lo = vmull_u16(vget_low_u16(a), vget_low_u16(b));
    uint32x4_t hi = vmull_u16(vget_high_u16(a), vget_high_u16(b));

    // Barrett reduction
    uint32x4_t inv_vec = vdupq_n_u32(inv);
    lo = vmulq_u32(lo, inv_vec);
    hi = vmulq_u32(hi, inv_vec);

    // Shift and combine
    lo = vshrq_n_u32(lo, 26); // Adjust shift based on SABER_Q
    hi = vshrq_n_u32(hi, 26);

    uint16x4_t res_lo = vmovn_u32(lo);
    uint16x4_t res_hi = vmovn_u32(hi);

    return vcombine_u16(res_lo, res_hi);
}

#endif // NEON_BATCH_POLY_H