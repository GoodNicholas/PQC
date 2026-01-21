/*
 * Implementation of core functions for GOST R 34.11-2012 using ARM NEON.
 *
 * Ported from SSE2 version by Alexey Degtyarev <alexey@renatasystems.org>
 * ARM NEON port: 2026
 *
 * This file is distributed under the same license as OpenSSL.
 */

#ifndef __GOST3411_HAS_NEON__
# error "GOST R 34.11-2012: ARM NEON not enabled"
#endif

#include <arm_neon.h>

/* Helper macros for byte extraction */
#define LO(v) ((unsigned char) (v))
#define HI(v) ((unsigned char) (((unsigned int) (v)) >> 8))

/* Use 64-bit extraction on AArch64 */
#ifdef __aarch64__
# define EXTRACT EXTRACT64
#else
# define EXTRACT EXTRACT32
#endif

/* ========================================================================
 * NEON Memory Operations (128-bit vectors)
 * ======================================================================== */

/* Load 512 bits (4 × 128-bit vectors) from unaligned memory */
#define ULOAD(P, v0, v1, v2, v3) { \
    const uint8_t *__p = (const uint8_t *) P; \
    v0 = vld1q_u8(__p + 0*16); \
    v1 = vld1q_u8(__p + 1*16); \
    v2 = vld1q_u8(__p + 2*16); \
    v3 = vld1q_u8(__p + 3*16); \
}

/* Aligned vs unaligned access */
#ifdef UNALIGNED_SIMD_ACCESS
# define MEM_WRITE_U8X16 vst1q_u8
# define MEM_READ_U8X16  vld1q_u8
# define LOAD            ULOAD
#else
/* On ARM, unaligned access is typically efficient, but provide aligned version */
# define MEM_WRITE_U8X16 vst1q_u8
# define MEM_READ_U8X16  vld1q_u8
# define LOAD(P, v0, v1, v2, v3) { \
    const uint8_t *__p = (const uint8_t *) P; \
    v0 = vld1q_u8(__p + 0*16); \
    v1 = vld1q_u8(__p + 1*16); \
    v2 = vld1q_u8(__p + 2*16); \
    v3 = vld1q_u8(__p + 3*16); \
}
#endif

/* Store 512 bits (4 × 128-bit vectors) to memory */
#define STORE(P, v0, v1, v2, v3) { \
    uint8_t *__p = (uint8_t *) &P[0]; \
    vst1q_u8(__p + 0*16, v0); \
    vst1q_u8(__p + 1*16, v1); \
    vst1q_u8(__p + 2*16, v2); \
    vst1q_u8(__p + 3*16, v3); \
}

/* ========================================================================
 * XOR Operations on 4 × 128-bit vectors
 * ======================================================================== */

/* XOR between two sets of 4 vectors: v0-v3 ^= v4-v7 */
#define X128R(v0, v1, v2, v3, v4, v5, v6, v7) { \
    v0 = veorq_u8(v0, v4); \
    v1 = veorq_u8(v1, v5); \
    v2 = veorq_u8(v2, v6); \
    v3 = veorq_u8(v3, v7); \
}

/* XOR with memory: v0-v3 ^= P[0-3] */
#define X128M(P, v0, v1, v2, v3) { \
    const uint8_t *__p = (const uint8_t *) &P[0]; \
    v0 = veorq_u8(v0, vld1q_u8(__p + 0*16)); \
    v1 = veorq_u8(v1, vld1q_u8(__p + 1*16)); \
    v2 = veorq_u8(v2, vld1q_u8(__p + 2*16)); \
    v3 = veorq_u8(v3, vld1q_u8(__p + 3*16)); \
}

/* ========================================================================
 * EXTRACT Macros - Table Lookup Implementation
 *
 * Streebog uses large precomputed tables (Ax[8][256]) for S-box and
 * linear transformation. Each 16-bit value is split into LO/HI bytes,
 * used as indices into the tables, and results are XORed together.
 * ======================================================================== */

/* Helper macro to extract with constant lane index - needed for both architectures */
#define EXTRACT_LANE(v, lane) vgetq_lane_u16(vreinterpretq_u16_u8(v), lane)

#ifdef __aarch64__

/* 64-bit extraction - fully unrolled for row=0 */
#define EXTRACT64_0(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 0); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 4); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 0); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 4); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 0); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 4); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 0); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 4); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

/* 64-bit extraction - fully unrolled for row=1 */
#define EXTRACT64_1(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 1); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 5); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 1); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 5); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 1); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 5); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 1); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 5); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

/* 64-bit extraction - fully unrolled for row=2 */
#define EXTRACT64_2(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 2); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 6); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 2); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 6); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 2); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 6); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 2); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 6); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

/* 64-bit extraction - fully unrolled for row=3 */
#define EXTRACT64_3(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 3); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 7); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 3); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 7); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 3); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 7); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 3); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 7); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

/* Dispatcher macro for EXTRACT64 */
#define EXTRACT64(row, v0, v1, v2, v3, v4) EXTRACT64_##row(v0, v1, v2, v3, v4)

#else /* !__aarch64__ - 32-bit ARM */

/* 32-bit extraction - fully unrolled versions for ARMv7 */
#define EXTRACT32_0(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 0); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 4); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 0); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 4); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 0); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 4); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 0); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 4); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

#define EXTRACT32_1(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 1); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 5); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 1); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 5); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 1); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 5); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 1); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 5); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

#define EXTRACT32_2(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 2); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 6); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 2); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 6); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 2); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 6); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 2); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 6); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

#define EXTRACT32_3(v0, v1, v2, v3, v4) { \
    register unsigned short ax; \
    register unsigned long long r0, r1; \
    \
    ax = (unsigned short) EXTRACT_LANE(v0, 3); \
    r0  = Ax[0][LO(ax)]; \
    r1  = Ax[0][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v0, 7); \
    r0 ^= Ax[1][LO(ax)]; \
    r1 ^= Ax[1][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 3); \
    r0 ^= Ax[2][LO(ax)]; \
    r1 ^= Ax[2][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v1, 7); \
    r0 ^= Ax[3][LO(ax)]; \
    r1 ^= Ax[3][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 3); \
    r0 ^= Ax[4][LO(ax)]; \
    r1 ^= Ax[4][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v2, 7); \
    r0 ^= Ax[5][LO(ax)]; \
    r1 ^= Ax[5][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 3); \
    r0 ^= Ax[6][LO(ax)]; \
    r1 ^= Ax[6][HI(ax)]; \
    ax = (unsigned short) EXTRACT_LANE(v3, 7); \
    r0 ^= Ax[7][LO(ax)]; \
    r1 ^= Ax[7][HI(ax)]; \
    \
    uint64x2_t tmp = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1)); \
    v4 = vreinterpretq_u8_u64(tmp); \
}

/* Dispatcher macro for EXTRACT32 */
#define EXTRACT32(row, v0, v1, v2, v3, v4) EXTRACT32_##row(v0, v1, v2, v3, v4)

#endif /* __aarch64__ */

/* ========================================================================
 * High-Level Streebog Operations
 * ======================================================================== */

/* XOR with memory, then apply linear transformation (LPS) */
#define XLPS128M(P, v0, v1, v2, v3) { \
    uint8x16_t tmp0, tmp1, tmp2, tmp3; \
    X128M(P, v0, v1, v2, v3); \
    \
    EXTRACT(0, v0, v1, v2, v3, tmp0); \
    EXTRACT(1, v0, v1, v2, v3, tmp1); \
    EXTRACT(2, v0, v1, v2, v3, tmp2); \
    EXTRACT(3, v0, v1, v2, v3, tmp3); \
    \
    v0 = tmp0; \
    v1 = tmp1; \
    v2 = tmp2; \
    v3 = tmp3; \
}

/* XOR between vectors, then apply linear transformation (LPS) */
#define XLPS128R(v0, v1, v2, v3, v4, v5, v6, v7) { \
    uint8x16_t tmp0, tmp1, tmp2, tmp3; \
    X128R(v4, v5, v6, v7, v0, v1, v2, v3); \
    \
    EXTRACT(0, v4, v5, v6, v7, tmp0); \
    EXTRACT(1, v4, v5, v6, v7, tmp1); \
    EXTRACT(2, v4, v5, v6, v7, tmp2); \
    EXTRACT(3, v4, v5, v6, v7, tmp3); \
    \
    v4 = tmp0; \
    v5 = tmp1; \
    v6 = tmp2; \
    v7 = tmp3; \
}

/* Round function with key addition */
#define ROUND128(i, v0, v2, v4, v6, v1, v3, v5, v7) { \
    XLPS128M((&C[i]), v0, v2, v4, v6); \
    XLPS128R(v0, v2, v4, v6, v1, v3, v5, v7); \
}

/* ========================================================================
 * Notes on ARM NEON vs SSE2 Mapping
 * ========================================================================
 *
 * SSE2                  ARM NEON                   Notes
 * ----------------      ------------------         ---------------------
 * __m128i               uint8x16_t                 128-bit vector
 * _mm_load_si128        vld1q_u8                   Aligned load
 * _mm_loadu_si128       vld1q_u8                   Unaligned load (same on ARM)
 * _mm_store_si128       vst1q_u8                   Aligned store
 * _mm_storeu_si128      vst1q_u8                   Unaligned store (same)
 * _mm_xor_si128         veorq_u8                   XOR operation
 * _mm_extract_epi16     vgetq_lane_u16             Extract 16-bit lane
 * _mm_cvtsi64_si128     vcreate_u64                Create from 64-bit
 * _mm_unpacklo_epi64    vcombine_u64               Combine two 64-bit
 *
 * Key differences:
 * 1. ARM NEON uses typed intrinsics (u8, u16, u32, u64)
 * 2. ARM unaligned access is typically as fast as aligned
 * 3. No separate _mm_lddqu_si128 needed on ARM
 * 4. Table lookup can use vtbl/vqtbl (not used here for compatibility)
 */
