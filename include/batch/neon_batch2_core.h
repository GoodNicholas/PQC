/**
 * @file neon_batch2_core.h
 * @brief TRUE NEON batching interface - Core operations
 *
 * Real parallel processing, no fake sequential loops
 */

#ifndef NEON_BATCH2_CORE_H
#define NEON_BATCH2_CORE_H

#include <stdint.h>
#include <stddef.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// POLYNOMIAL ARITHMETIC - TRUE PARALLEL
// ============================================================================

/**
 * @brief Add 2 polynomial pairs in parallel
 *
 * c0 = a0 + b0, c1 = a1 + b1 (executed simultaneously)
 */
void neon_batch2_poly_add(
    uint16_t *c0, uint16_t *c1,
    const uint16_t *a0, const uint16_t *a1,
    const uint16_t *b0, const uint16_t *b1
);

/**
 * @brief Subtract 2 polynomial pairs in parallel
 *
 * c0 = a0 - b0, c1 = a1 - b1 (executed simultaneously)
 */
void neon_batch2_poly_sub(
    uint16_t *c0, uint16_t *c1,
    const uint16_t *a0, const uint16_t *a1,
    const uint16_t *b0, const uint16_t *b1
);

/**
 * @brief Parallel modular reduction for 2 polynomials
 */
void neon_batch2_poly_reduce(
    uint16_t *a0, uint16_t *a1,
    uint16_t modulus
);

/**
 * @brief Multiply 2 polynomial pairs in parallel (schoolbook)
 *
 * c0 = a0 * b0, c1 = a1 * b1 (executed simultaneously)
 *
 * @param c0, c1 Output polynomials (size 2n)
 * @param a0, a1, b0, b1 Input polynomials (size n)
 * @param n Polynomial degree + 1
 */
void neon_batch2_poly_mul_schoolbook(
    uint16_t *c0, uint16_t *c1,
    const uint16_t *a0, const uint16_t *a1,
    const uint16_t *b0, const uint16_t *b1,
    size_t n
);

// ============================================================================
// MATRIX-VECTOR OPERATIONS - TRUE PARALLEL
// ============================================================================

/**
 * @brief Matrix-vector multiplication for 2 vectors with shared matrix
 *
 * res0 = A * s0, res1 = A * s1 (parallel execution)
 *
 * This is the CRITICAL operation for SABER performance.
 * The matrix A is loaded once and used for both multiplications.
 *
 * @param res0, res1 Output vectors [SABER_L][SABER_N]
 * @param A Shared matrix [SABER_L][SABER_L][SABER_N]
 * @param s0, s1 Input vectors [SABER_L][SABER_N]
 */
void neon_batch2_matrix_vector_mul(
    uint16_t res0[SABER_L][SABER_N],
    uint16_t res1[SABER_L][SABER_N],
    const uint16_t A[SABER_L][SABER_L][SABER_N],
    const uint16_t s0[SABER_L][SABER_N],
    const uint16_t s1[SABER_L][SABER_N]
);

/**
 * @brief Inner product for 2 vector pairs
 *
 * res0 = <b0, s0>, res1 = <b1, s1> (parallel execution)
 *
 * @param res0, res1 Output polynomials [SABER_N]
 * @param b0, b1, s0, s1 Input vectors [SABER_L][SABER_N]
 */
void neon_batch2_inner_product(
    uint16_t res0[SABER_N],
    uint16_t res1[SABER_N],
    const uint16_t b0[SABER_L][SABER_N],
    const uint16_t b1[SABER_L][SABER_N],
    const uint16_t s0[SABER_L][SABER_N],
    const uint16_t s1[SABER_L][SABER_N]
);

// ============================================================================
// DATA TRANSFORMATION
// ============================================================================

/**
 * @brief Interleave coefficients from 2 polynomials
 *
 * Optimizes memory layout for NEON processing
 *
 * @param interleaved Output (size 2n)
 * @param a0, a1 Input polynomials (size n)
 * @param n Number of coefficients
 */
void neon_batch2_interleave(
    uint16_t *interleaved,
    const uint16_t *a0,
    const uint16_t *a1,
    size_t n
);

/**
 * @brief De-interleave coefficients back to separate polynomials
 *
 * @param a0, a1 Output polynomials (size n)
 * @param interleaved Input (size 2n)
 * @param n Number of coefficients per polynomial
 */
void neon_batch2_deinterleave(
    uint16_t *a0,
    uint16_t *a1,
    const uint16_t *interleaved,
    size_t n
);

#ifdef __cplusplus
}
#endif

#endif // NEON_BATCH2_CORE_H