#ifndef NEON_POLY_BATCH_H
#define NEON_POLY_BATCH_H

#include <stdint.h>
#include "SABER_params.h"

/*
 * NEON-batched polynomial operations for SaberX2
 * All functions process 2 polynomial instances simultaneously
 */

// Batched rounding: (src + h1) >> shift
void poly_round_2x(uint16_t des0[SABER_N], uint32_t src0[SABER_N],
                   uint16_t des1[SABER_N], uint32_t src1[SABER_N],
                   int h1_val, int shift);

// Batched encoding with message
void poly_enc_add_msg_2x(uint16_t cipher0[SABER_N], uint32_t src0[SABER_N], uint16_t msg0[SABER_N],
                         uint16_t cipher1[SABER_N], uint32_t src1[SABER_N], uint16_t msg1[SABER_N],
                         int h1_val, int msg_shift, uint32_t mask_val);

// Batched 16->32 bit conversion
void poly_16_to_32_2x(uint32_t dst0[SABER_N], const uint16_t src0[SABER_N],
                      uint32_t dst1[SABER_N], const uint16_t src1[SABER_N]);

// Batched addition
void poly_add_2x(uint16_t dst0[SABER_N], const uint16_t src_a0[SABER_N], const uint16_t src_b0[SABER_N],
                 uint16_t dst1[SABER_N], const uint16_t src_a1[SABER_N], const uint16_t src_b1[SABER_N]);

// Batched subtraction
void poly_sub_2x(uint16_t dst0[SABER_N], const uint16_t src_a0[SABER_N], const uint16_t src_b0[SABER_N],
                 uint16_t dst1[SABER_N], const uint16_t src_a1[SABER_N], const uint16_t src_b1[SABER_N]);

#endif // NEON_POLY_BATCH_H
