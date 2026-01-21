/*
 * SaberX4 NEON - 4x batching using 2x shake128x2 calls
 * Based on SaberX4 AVX2 (https://github.com/sujoyetc/SaberX4)
 *
 * Note: This is "pseudo-4x" using two shake128x2 calls sequentially,
 * not true 4-way parallel like AVX2 shake128x4.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "SABER_params.h"
#include "pack_unpack.h"
#include "poly.h"
#include "fips202.h"

// Use existing shake128x2 from neon-ntt or batch implementation
extern void shake128x2(uint8_t *out0, uint8_t *out1, size_t outlen,
                       const uint8_t *in0, const uint8_t *in1, size_t inlen);

// Compatibility macros (same as saberx2)
#define SABER_K SABER_L
#define SABER_Q (1 << SABER_EQ)
#define SABER_P (1 << SABER_EP)
#define SABER_COINBYTES SABER_POLYCOINBYTES
#define SABER_NOISESEEDBYTES SABER_NOISE_SEEDBYTES

// External functions from reference implementation
extern void BS2POLq(const unsigned char *bytes, uint16_t data[SABER_N]);
extern void BS2POLmsg(const unsigned char *bytes, uint16_t data[SABER_N]);
extern void BS2POLp(const unsigned char *bytes, uint16_t data[SABER_N]);
extern void POLVECq2BS(uint8_t *bytes, const uint16_t data[SABER_K][SABER_N]);
extern void POLVECp2BS(uint8_t *bytes, const uint16_t data[SABER_K][SABER_N]);
extern void POLmsg2BS(uint8_t *bytes, const uint16_t data[SABER_N]);
extern void poly_mul_acc(const uint16_t a[SABER_N], const uint16_t b[SABER_N], uint16_t res[SABER_N]);

/*-----------------------------------------------------------------------------------
    GenMatrix4x: Generate 4 public matrices in parallel using 2x shake128x2
-------------------------------------------------------------------------------------*/
static void GenMatrix4x(
    uint16_t A0[SABER_K][SABER_K][SABER_N],
    uint16_t A1[SABER_K][SABER_K][SABER_N],
    uint16_t A2[SABER_K][SABER_K][SABER_N],
    uint16_t A3[SABER_K][SABER_K][SABER_N],
    const unsigned char *seed0,
    const unsigned char *seed1,
    const unsigned char *seed2,
    const unsigned char *seed3)
{
    unsigned int one_vector = 13 * SABER_N / 8;
    unsigned int byte_bank_length = SABER_K * SABER_K * one_vector;

    // Use malloc to avoid stack overflow on ARM
    unsigned char *buf0 = malloc(byte_bank_length);
    unsigned char *buf1 = malloc(byte_bank_length);
    unsigned char *buf2 = malloc(byte_bank_length);
    unsigned char *buf3 = malloc(byte_bank_length);

    if (!buf0 || !buf1 || !buf2 || !buf3) {
        free(buf0); free(buf1); free(buf2); free(buf3);
        return; // Error handling
    }

    // First pair: keypairs 0,1 using shake128x2
    shake128x2(buf0, buf1, byte_bank_length, seed0, seed1, SABER_SEEDBYTES);

    // Second pair: keypairs 2,3 using shake128x2
    shake128x2(buf2, buf3, byte_bank_length, seed2, seed3, SABER_SEEDBYTES);

    // Unpack all 4 matrices
    for (int i = 0; i < SABER_K; i++) {
        for (int j = 0; j < SABER_K; j++) {
            BS2POLq(buf0 + (i * SABER_K + j) * one_vector, A0[i][j]);
            BS2POLq(buf1 + (i * SABER_K + j) * one_vector, A1[i][j]);
            BS2POLq(buf2 + (i * SABER_K + j) * one_vector, A2[i][j]);
            BS2POLq(buf3 + (i * SABER_K + j) * one_vector, A3[i][j]);
        }
    }

    free(buf0);
    free(buf1);
    free(buf2);
    free(buf3);
}

/*-----------------------------------------------------------------------------------
    GenSecret4x: Generate 4 secret vectors in parallel using 2x shake128x2
-------------------------------------------------------------------------------------*/
static void GenSecret4x(
    uint16_t s0[SABER_K][SABER_N],
    uint16_t s1[SABER_K][SABER_N],
    uint16_t s2[SABER_K][SABER_N],
    uint16_t s3[SABER_K][SABER_N],
    const unsigned char *seed0,
    const unsigned char *seed1,
    const unsigned char *seed2,
    const unsigned char *seed3)
{
    unsigned int buf_size = SABER_K * SABER_COINBYTES;

    unsigned char *buf0 = malloc(buf_size);
    unsigned char *buf1 = malloc(buf_size);
    unsigned char *buf2 = malloc(buf_size);
    unsigned char *buf3 = malloc(buf_size);

    if (!buf0 || !buf1 || !buf2 || !buf3) {
        free(buf0); free(buf1); free(buf2); free(buf3);
        return;
    }

    // First pair: keypairs 0,1
    shake128x2(buf0, buf1, buf_size, seed0, seed1, SABER_NOISESEEDBYTES);

    // Second pair: keypairs 2,3
    shake128x2(buf2, buf3, buf_size, seed2, seed3, SABER_NOISESEEDBYTES);

    // CBD sampling for all 4 secrets
    extern void cbd(uint16_t *s, const unsigned char *buf);
    for (int i = 0; i < SABER_K; i++) {
        cbd(s0[i], buf0 + i * SABER_COINBYTES);
        cbd(s1[i], buf1 + i * SABER_COINBYTES);
        cbd(s2[i], buf2 + i * SABER_COINBYTES);
        cbd(s3[i], buf3 + i * SABER_COINBYTES);
    }

    free(buf0);
    free(buf1);
    free(buf2);
    free(buf3);
}

/*-----------------------------------------------------------------------------------
    Matrix-vector multiplication (same as reference, but for one keypair)
-------------------------------------------------------------------------------------*/
static void MatrixVectorMul(
    const uint16_t A[SABER_K][SABER_K][SABER_N],
    const uint16_t s[SABER_K][SABER_N],
    uint16_t res[SABER_K][SABER_N],
    int16_t transpose)
{
    for (int i = 0; i < SABER_K; i++) {
        for (int k = 0; k < SABER_N; k++) {
            res[i][k] = 0;
        }

        for (int j = 0; j < SABER_K; j++) {
            uint16_t acc[SABER_N];
            if (transpose) {
                poly_mul_acc(A[j][i], s[j], acc);
            } else {
                poly_mul_acc(A[i][j], s[j], acc);
            }

            for (int k = 0; k < SABER_N; k++) {
                res[i][k] += acc[k];
            }
        }
    }
}

/*-----------------------------------------------------------------------------------
    indcpa_kem_keypair_x4: Generate 4 keypairs simultaneously
-------------------------------------------------------------------------------------*/
void indcpa_kem_keypair_x4(
    uint8_t *pk0, uint8_t *sk0,
    uint8_t *pk1, uint8_t *sk1,
    uint8_t *pk2, uint8_t *sk2,
    uint8_t *pk3, uint8_t *sk3)
{
    // Allocate matrices and vectors on heap to avoid stack overflow
    uint16_t (*A0)[SABER_K][SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_K * SABER_N);
    uint16_t (*A1)[SABER_K][SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_K * SABER_N);
    uint16_t (*A2)[SABER_K][SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_K * SABER_N);
    uint16_t (*A3)[SABER_K][SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_K * SABER_N);

    uint16_t (*s0)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);
    uint16_t (*s1)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);
    uint16_t (*s2)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);
    uint16_t (*s3)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);

    uint16_t (*b0)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);
    uint16_t (*b1)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);
    uint16_t (*b2)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);
    uint16_t (*b3)[SABER_N] = malloc(sizeof(uint16_t) * SABER_K * SABER_N);

    unsigned char seed0[SABER_SEEDBYTES], seed1[SABER_SEEDBYTES];
    unsigned char seed2[SABER_SEEDBYTES], seed3[SABER_SEEDBYTES];
    unsigned char noiseseed0[SABER_NOISESEEDBYTES], noiseseed1[SABER_NOISESEEDBYTES];
    unsigned char noiseseed2[SABER_NOISESEEDBYTES], noiseseed3[SABER_NOISESEEDBYTES];

    extern void randombytes(unsigned char *buf, size_t n);

    // Generate seeds
    randombytes(seed0, SABER_SEEDBYTES);
    randombytes(noiseseed0, SABER_NOISESEEDBYTES);
    randombytes(seed1, SABER_SEEDBYTES);
    randombytes(noiseseed1, SABER_NOISESEEDBYTES);
    randombytes(seed2, SABER_SEEDBYTES);
    randombytes(noiseseed2, SABER_NOISESEEDBYTES);
    randombytes(seed3, SABER_SEEDBYTES);
    randombytes(noiseseed3, SABER_NOISESEEDBYTES);

    // Hash seeds
    shake128(seed0, SABER_SEEDBYTES, seed0, SABER_SEEDBYTES);
    shake128(seed1, SABER_SEEDBYTES, seed1, SABER_SEEDBYTES);
    shake128(seed2, SABER_SEEDBYTES, seed2, SABER_SEEDBYTES);
    shake128(seed3, SABER_SEEDBYTES, seed3, SABER_SEEDBYTES);

    // BATCHED: Generate matrices and secrets for 4 keypairs
    GenMatrix4x(A0, A1, A2, A3, seed0, seed1, seed2, seed3);
    GenSecret4x(s0, s1, s2, s3, noiseseed0, noiseseed1, noiseseed2, noiseseed3);

    // SERIAL: Matrix-vector multiplication for each keypair
    MatrixVectorMul(A0, s0, b0, 1);
    MatrixVectorMul(A1, s1, b1, 1);
    MatrixVectorMul(A2, s2, b2, 1);
    MatrixVectorMul(A3, s3, b3, 1);

    // Round and pack
    for (int i = 0; i < SABER_K; i++) {
        for (int j = 0; j < SABER_N; j++) {
            b0[i][j] = (b0[i][j] + (1 << (SABER_EQ - SABER_EP - 1))) >> (SABER_EQ - SABER_EP);
            b1[i][j] = (b1[i][j] + (1 << (SABER_EQ - SABER_EP - 1))) >> (SABER_EQ - SABER_EP);
            b2[i][j] = (b2[i][j] + (1 << (SABER_EQ - SABER_EP - 1))) >> (SABER_EQ - SABER_EP);
            b3[i][j] = (b3[i][j] + (1 << (SABER_EQ - SABER_EP - 1))) >> (SABER_EQ - SABER_EP);
        }
    }

    // Pack public keys
    POLVECp2BS(pk0, b0);
    memcpy(pk0 + SABER_POLYVECCOMPRESSEDBYTES, seed0, SABER_SEEDBYTES);

    POLVECp2BS(pk1, b1);
    memcpy(pk1 + SABER_POLYVECCOMPRESSEDBYTES, seed1, SABER_SEEDBYTES);

    POLVECp2BS(pk2, b2);
    memcpy(pk2 + SABER_POLYVECCOMPRESSEDBYTES, seed2, SABER_SEEDBYTES);

    POLVECp2BS(pk3, b3);
    memcpy(pk3 + SABER_POLYVECCOMPRESSEDBYTES, seed3, SABER_SEEDBYTES);

    // Pack secret keys
    POLVECq2BS(sk0, s0);
    POLVECq2BS(sk1, s1);
    POLVECq2BS(sk2, s2);
    POLVECq2BS(sk3, s3);

    // Cleanup
    free(A0); free(A1); free(A2); free(A3);
    free(s0); free(s1); free(s2); free(s3);
    free(b0); free(b1); free(b2); free(b3);
}
