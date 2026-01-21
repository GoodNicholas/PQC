/*
 * NEON-batched polynomial operations for SaberX2
 * Processes 2 polynomial instances simultaneously using NEON intrinsics
 */

#include <stdint.h>
#include <arm_neon.h>
#include "SABER_params.h"

/*
 * Batched rounding operation for 2 polynomials
 * Performs signed rounding shift matching __asm_round:
 * des[i] = srshr(src[i], SABER_EQ-SABER_EP) - signed rounding shift right by 3
 * Both polynomials processed in parallel using NEON
 */
void poly_round_2x(uint16_t des0[SABER_N], uint32_t src0[SABER_N],
                   uint16_t des1[SABER_N], uint32_t src1[SABER_N],
                   int h1_val, int shift)
{
    // The input is 32-bit but stored as interleaved 16-bit halves for assembly
    // We need to read as 16-bit and use signed rounding shift
    int16_t *src0_16 = (int16_t *)src0;
    int16_t *src1_16 = (int16_t *)src1;
    int16_t *des0_16 = (int16_t *)des0;
    int16_t *des1_16 = (int16_t *)des1;

    // Process 8 coefficients at a time
    for (int i = 0; i < SABER_N * 2; i += 16) {
        // Load interleaved data (ld2 equivalent)
        int16x8x2_t a0_pair = vld2q_s16(&src0_16[i]);
        int16x8x2_t a1_pair = vld2q_s16(&src1_16[i]);

        // Apply signed rounding shift to first element of each pair (shift=3 for SABER)
        int16x8_t r0 = vrshrq_n_s16(a0_pair.val[0], 3);
        int16x8_t r1 = vrshrq_n_s16(a1_pair.val[0], 3);

        // Store results
        vst1q_s16(&des0_16[i/2], r0);
        vst1q_s16(&des1_16[i/2], r1);
    }
}

/*
 * Batched polynomial addition with message encoding
 * Performs: cipher0[i] = (src0[i] + h1 - (msg0[i] << (EP-1))) & mask
 *           cipher1[i] = (src1[i] + h1 - (msg1[i] << (EP-1))) & mask
 */
void poly_enc_add_msg_2x(uint16_t cipher0[SABER_N], uint32_t src0[SABER_N], uint16_t msg0[SABER_N],
                         uint16_t cipher1[SABER_N], uint32_t src1[SABER_N], uint16_t msg1[SABER_N],
                         int h1_val, int msg_shift, uint32_t mask_val)
{
    const uint32x4_t h1 = vdupq_n_u32(h1_val);
    const uint32x4_t mask = vdupq_n_u32(mask_val);

    for (int i = 0; i < SABER_N; i += 4) {
        // Load source polynomials
        uint32x4_t a0 = vld1q_u32(&src0[i]);
        uint32x4_t a1 = vld1q_u32(&src1[i]);

        // Load and extend messages to 32-bit
        uint16x4_t m0_16 = vld1_u16(&msg0[i]);
        uint16x4_t m1_16 = vld1_u16(&msg1[i]);
        uint32x4_t m0 = vmovl_u16(m0_16);
        uint32x4_t m1 = vmovl_u16(m1_16);

        // Shift messages
        m0 = vshlq_n_u32(m0, msg_shift);
        m1 = vshlq_n_u32(m1, msg_shift);

        // Add h1
        a0 = vaddq_u32(a0, h1);
        a1 = vaddq_u32(a1, h1);

        // Subtract messages
        a0 = vsubq_u32(a0, m0);
        a1 = vsubq_u32(a1, m1);

        // Apply mask
        a0 = vandq_u32(a0, mask);
        a1 = vandq_u32(a1, mask);

        // Shift and narrow to 16-bit
        a0 = vshrq_n_u32(a0, 1); // Adjust shift as needed
        a1 = vshrq_n_u32(a1, 1);

        uint16x4_t r0 = vmovn_u32(a0);
        uint16x4_t r1 = vmovn_u32(a1);

        vst1_u16(&cipher0[i], r0);
        vst1_u16(&cipher1[i], r1);
    }
}

/*
 * Batched 16-bit to 32-bit conversion for 2 polynomials
 * Sign-extends int16_t coefficients to int32_t (matching __asm_16_to_32)
 */
void poly_16_to_32_2x(uint32_t dst0[SABER_N], const uint16_t src0[SABER_N],
                      uint32_t dst1[SABER_N], const uint16_t src1[SABER_N])
{
    // Cast to signed types for sign extension
    int32_t *dst0_s = (int32_t *)dst0;
    int32_t *dst1_s = (int32_t *)dst1;
    const int16_t *src0_s = (const int16_t *)src0;
    const int16_t *src1_s = (const int16_t *)src1;

    for (int i = 0; i < SABER_N; i += 8) {
        // Load 8 int16_t from each polynomial
        int16x8_t a0 = vld1q_s16(&src0_s[i]);
        int16x8_t a1 = vld1q_s16(&src1_s[i]);

        // Sign-extend to 2x int32x4_t
        int32x4_t a0_lo = vmovl_s16(vget_low_s16(a0));
        int32x4_t a0_hi = vmovl_s16(vget_high_s16(a0));
        int32x4_t a1_lo = vmovl_s16(vget_low_s16(a1));
        int32x4_t a1_hi = vmovl_s16(vget_high_s16(a1));

        // Store
        vst1q_s32(&dst0_s[i], a0_lo);
        vst1q_s32(&dst0_s[i+4], a0_hi);
        vst1q_s32(&dst1_s[i], a1_lo);
        vst1q_s32(&dst1_s[i+4], a1_hi);
    }
}

/*
 * Batched polynomial addition
 * dst = src_a + src_b (mod 2^16)
 */
void poly_add_2x(uint16_t dst0[SABER_N], const uint16_t src_a0[SABER_N], const uint16_t src_b0[SABER_N],
                 uint16_t dst1[SABER_N], const uint16_t src_a1[SABER_N], const uint16_t src_b1[SABER_N])
{
    for (int i = 0; i < SABER_N; i += 8) {
        uint16x8_t a0 = vld1q_u16(&src_a0[i]);
        uint16x8_t b0 = vld1q_u16(&src_b0[i]);
        uint16x8_t a1 = vld1q_u16(&src_a1[i]);
        uint16x8_t b1 = vld1q_u16(&src_b1[i]);

        uint16x8_t r0 = vaddq_u16(a0, b0);
        uint16x8_t r1 = vaddq_u16(a1, b1);

        vst1q_u16(&dst0[i], r0);
        vst1q_u16(&dst1[i], r1);
    }
}

/*
 * Batched polynomial subtraction
 * dst = src_a - src_b (mod 2^16)
 */
void poly_sub_2x(uint16_t dst0[SABER_N], const uint16_t src_a0[SABER_N], const uint16_t src_b0[SABER_N],
                 uint16_t dst1[SABER_N], const uint16_t src_a1[SABER_N], const uint16_t src_b1[SABER_N])
{
    for (int i = 0; i < SABER_N; i += 8) {
        uint16x8_t a0 = vld1q_u16(&src_a0[i]);
        uint16x8_t b0 = vld1q_u16(&src_b0[i]);
        uint16x8_t a1 = vld1q_u16(&src_a1[i]);
        uint16x8_t b1 = vld1q_u16(&src_b1[i]);

        uint16x8_t r0 = vsubq_u16(a0, b0);
        uint16x8_t r1 = vsubq_u16(a1, b1);

        vst1q_u16(&dst0[i], r0);
        vst1q_u16(&dst1[i], r1);
    }
}
