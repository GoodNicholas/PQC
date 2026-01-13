/**
 * @file pack_unpack_neon.h
 * @brief NEON-optimized pack/unpack operations for SABER
 */

#ifndef PACK_UNPACK_NEON_H
#define PACK_UNPACK_NEON_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * NEON Optimized Pack/Unpack Functions
 * ======================================================================== */

/**
 * BS2POLVECq_neon - Unpack vector of polynomials from bytes (13-bit coefficients)
 */
void BS2POLVECq_neon(const uint8_t bytes[SABER_POLYVECBYTES], uint16_t data[SABER_L][SABER_N]);

/**
 * POLVECq2BS_neon - Pack vector of polynomials to bytes (13-bit coefficients)
 */
void POLVECq2BS_neon(uint8_t bytes[SABER_POLYVECBYTES], const uint16_t data[SABER_L][SABER_N]);

/**
 * BS2POLVECp_neon - Unpack vector of polynomials from bytes (10-bit coefficients)
 */
void BS2POLVECp_neon(const uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES], uint16_t data[SABER_L][SABER_N]);

/**
 * POLVECp2BS_neon - Pack vector of polynomials to bytes (10-bit coefficients)
 */
void POLVECp2BS_neon(uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES], const uint16_t data[SABER_L][SABER_N]);

#ifdef __cplusplus
}
#endif

#endif /* PACK_UNPACK_NEON_H */