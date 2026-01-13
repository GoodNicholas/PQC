/**
 * @file poly_genmatrix.c
 * @brief Minimal GenMatrix implementation for FAST_V4 configuration
 *
 * FAST_V4 uses neon-ntt's SABER_indcpa.c which doesn't provide GenMatrix,
 * but hash_sha3.c needs GenMatrix for gen_matrix_A().
 *
 * This file provides ONLY GenMatrix, extracted from SABER reference poly.c,
 * without the MatrixVectorMul and InnerProd functions that depend on poly_mul_acc.
 */

#include <stdio.h>
#include "../../../SABER/Reference_Implementation_KEM/api.h"
#include "../../../SABER/Reference_Implementation_KEM/poly.h"
#include "../../../SABER/Reference_Implementation_KEM/pack_unpack.h"
#include "../../../SABER/Reference_Implementation_KEM/fips202.h"

/**
 * GenMatrix - Generate public matrix A from seed
 *
 * @param A Output matrix [SABER_L][SABER_L][SABER_N]
 * @param seed Input seed [SABER_SEEDBYTES]
 *
 * Uses SHAKE128 to expand seed into matrix coefficients.
 */
void GenMatrix(uint16_t A[SABER_L][SABER_L][SABER_N], const uint8_t seed[SABER_SEEDBYTES])
{
	uint8_t buf[SABER_L * SABER_POLYVECBYTES];
	int i;

	shake128(buf, sizeof(buf), seed, SABER_SEEDBYTES);

	for (i = 0; i < SABER_L; i++)
	{
		BS2POLVECq(buf + i * SABER_POLYVECBYTES, A[i]);
	}
}
