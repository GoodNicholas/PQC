/**
 * @file poly_toom.c
 * @brief Toom-Cook polynomial multiplication wrapper for SABER_GOST
 *
 * Wraps Reference_Implementation's polynomial operations (Toom-Cook 4-way).
 * This is the default polynomial implementation for DEFAULT, GOST, and TEST configurations.
 *
 * The Reference_Implementation uses Toom-Cook 4-way with Karatsuba for efficient
 * polynomial multiplication in Z_q[x]/(x^256 + 1) where q = 8192.
 *
 * Note: This file is mostly a placeholder. The actual poly_mul, MatrixVectorMul,
 * and InnerProd functions are provided by poly_mul.c from Reference_Implementation.
 */

#include "../../include/poly.h"
#include "../../include/config.h"
#include "../../include/params.h"
#include "../../external/saber_ref/poly.h"
#include "../../external/saber_ref/poly_mul.h"

/*
 * All polynomial operations are provided by Reference_Implementation/poly_mul.c:
 * - poly_mul_acc()
 * - MatrixVectorMul()
 * - InnerProd()
 *
 * These functions are directly called by our code via the included headers.
 */
