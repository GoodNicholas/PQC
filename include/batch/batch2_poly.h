/**
 * @file batch2_poly.h
 * @brief Real 2x batching for polynomial operations using ARM NEON
 *
 * Based on SaberX4 architecture (https://eprint.iacr.org/2019/1309)
 * adapted for ARM NEON (2x batching instead of 4x AVX2).
 *
 * KEY PRINCIPLE: Data from 2 polynomials is INTERLEAVED in NEON registers
 * and processed with SINGLE SIMD instructions, achieving true parallelism.
 *
 * Example for 8 coefficients:
 *   Input:  a0 = [a0[0], a0[1], a0[2], a0[3], ...]
 *           a1 = [a1[0], a1[1], a1[2], a1[3], ...]
 *
 *   Interleaved: [a0[0], a1[0], a0[1], a1[1], a0[2], a1[2], a0[3], a1[3]]
 *                  ^^^^^  ^^^^^  --- ONE vmlaq_u16 processes BOTH ---
 *
 * This is REAL batching, not sequential processing with shared data.
 */

#ifndef SABER_BATCH2_POLY_H
#define SABER_BATCH2_POLY_H

#include <stdint.h>
#include <stddef.h>
#include <arm_neon.h>
#include "../params.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// Data Layout Utilities
// ==============================================================================

/**
 * @brief Interleave two polynomials for batched NEON processing
 *
 * Converts:
 *   a0 = [a0[0], a0[1], ..., a0[255]]
 *   a1 = [a1[0], a1[1], ..., a1[255]]
 *
 * To:
 *   interleaved = [a0[0], a1[0], a0[1], a1[1], ..., a0[255], a1[255]]
 *
 * This layout allows ONE NEON instruction to process coefficients from BOTH polynomials.
 *
 * @param interleaved Output buffer (size: 2 * SABER_N)
 * @param a0 First polynomial (size: SABER_N)
 * @param a1 Second polynomial (size: SABER_N)
 */
void batch2_poly_interleave(
    uint16_t *interleaved,
    const uint16_t *a0,
    const uint16_t *a1
);

/**
 * @brief De-interleave batched polynomial data
 *
 * Reverse of batch2_poly_interleave().
 *
 * @param a0 Output: first polynomial (size: SABER_N)
 * @param a1 Output: second polynomial (size: SABER_N)
 * @param interleaved Input: interleaved data (size: 2 * SABER_N)
 */
void batch2_poly_deinterleave(
    uint16_t *a0,
    uint16_t *a1,
    const uint16_t *interleaved
);

// ==============================================================================
// Batched Polynomial Operations
// ==============================================================================

/**
 * @brief Batched polynomial addition: c0 = a0 + b0, c1 = a1 + b1
 *
 * REAL BATCHING: Uses interleaved data, ONE vaddq_u16 processes both additions.
 *
 * @param c0 Output: first result
 * @param c1 Output: second result
 * @param a0 Input: first operand of first addition
 * @param b0 Input: second operand of first addition
 * @param a1 Input: first operand of second addition
 * @param b1 Input: second operand of second addition
 */
void batch2_poly_add(
    uint16_t c0[SABER_N],
    uint16_t c1[SABER_N],
    const uint16_t a0[SABER_N],
    const uint16_t b0[SABER_N],
    const uint16_t a1[SABER_N],
    const uint16_t b1[SABER_N]
);

/**
 * @brief Batched polynomial subtraction: c0 = a0 - b0, c1 = a1 - b1
 */
void batch2_poly_sub(
    uint16_t c0[SABER_N],
    uint16_t c1[SABER_N],
    const uint16_t a0[SABER_N],
    const uint16_t b0[SABER_N],
    const uint16_t a1[SABER_N],
    const uint16_t b1[SABER_N]
);

/**
 * @brief Batched schoolbook multiplication for small polynomials
 *
 * Multiplies two pairs of n-coefficient polynomials using schoolbook algorithm.
 * Used as base case for Toom-Cook recursion.
 *
 * @param c0 Output: first product (size: 2*n)
 * @param c1 Output: second product (size: 2*n)
 * @param a0, a1 Inputs: first factors (size: n)
 * @param b0, b1 Inputs: second factors (size: n)
 * @param n Polynomial degree
 */
void batch2_poly_schoolbook(
    uint16_t *c0,
    uint16_t *c1,
    const uint16_t *a0,
    const uint16_t *a1,
    const uint16_t *b0,
    const uint16_t *b1,
    size_t n
);

/**
 * @brief Batched Toom-Cook 4-way polynomial multiplication
 *
 * Multiplies two pairs of 256-coefficient polynomials using Toom-Cook 4-way.
 * This is the MAIN batching function for DEFAULT configuration.
 *
 * Algorithm:
 *   1. Evaluation: Compute 7 evaluation points for both polynomial pairs (batched)
 *   2. Pointwise: Multiply at evaluation points using batch2_poly_schoolbook
 *   3. Interpolation: Recover product coefficients (batched)
 *
 * REAL BATCHING: All 3 phases use interleaved data and SIMD parallelism.
 *
 * @param c0 Output: first product (size: 2*SABER_N, but only [0..SABER_N) used)
 * @param c1 Output: second product (size: 2*SABER_N, but only [0..SABER_N) used)
 * @param a0, a1 Inputs: first factors (size: SABER_N)
 * @param b0, b1 Inputs: second factors (size: SABER_N)
 */
void batch2_poly_mul_toomcook(
    uint16_t c0[SABER_N],
    uint16_t c1[SABER_N],
    const uint16_t a0[SABER_N],
    const uint16_t a1[SABER_N],
    const uint16_t b0[SABER_N],
    const uint16_t b1[SABER_N]
);

// ==============================================================================
// Batched Matrix-Vector Operations
// ==============================================================================

/**
 * @brief Batched matrix-vector multiplication: res0 = A * s0, res1 = A * s1
 *
 * KEY OPTIMIZATION: Matrix A is SHARED between both operations.
 * Matrix loaded ONCE from memory, then multiplied with TWO vectors using
 * interleaved NEON operations.
 *
 * This is where batching provides maximum benefit (matrix dominates memory bandwidth).
 *
 * @param res0 Output: first result (LxN matrix)
 * @param res1 Output: second result (LxN matrix)
 * @param A Input: shared matrix (LxLxN tensor)
 * @param s0 Input: first vector (LxN matrix)
 * @param s1 Input: second vector (LxN matrix)
 */
void batch2_matrix_vector_mul(
    uint16_t res0[SABER_L][SABER_N],
    uint16_t res1[SABER_L][SABER_N],
    const uint16_t A[SABER_L][SABER_L][SABER_N],
    const uint16_t s0[SABER_L][SABER_N],
    const uint16_t s1[SABER_L][SABER_N]
);

/**
 * @brief Batched inner product: res0 = <a0, b0>, res1 = <a1, b1>
 *
 * Computes inner product of L polynomial vectors.
 *
 * @param res0 Output: first inner product (size: SABER_N)
 * @param res1 Output: second inner product (size: SABER_N)
 * @param a0, a1 Inputs: first vectors (size: SABER_L x SABER_N)
 * @param b0, b1 Inputs: second vectors (size: SABER_L x SABER_N)
 */
void batch2_inner_product(
    uint16_t res0[SABER_N],
    uint16_t res1[SABER_N],
    const uint16_t a0[SABER_L][SABER_N],
    const uint16_t b0[SABER_L][SABER_N],
    const uint16_t a1[SABER_L][SABER_N],
    const uint16_t b1[SABER_L][SABER_N]
);

#ifdef __cplusplus
}
#endif

#endif // SABER_BATCH2_POLY_H
