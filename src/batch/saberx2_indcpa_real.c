/*
 * SaberX2 - TRUE 2x batching implementation based on SaberX4 AVX2
 * Port to ARM NEON with shake128x2
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>
#include "saberx2_indcpa.h"
#include "../../external/saber_ref/SABER_params.h"
#include "../../external/saber_ref/pack_unpack.h"
#include "../../external/saber_ref/cbd.h"
#include "../../external/saber_ref/fips202.h"

// Compatibility with SaberX4 naming
#define SABER_K SABER_L
#define SABER_Q (1 << SABER_EQ)  // 2^13 = 8192
#define SABER_P (1 << SABER_EP)  // 2^10 = 1024
#define SABER_COINBYTES SABER_POLYCOINBYTES
#define SABER_NOISESEEDBYTES SABER_NOISE_SEEDBYTES

// External NEON batched functions
extern void shake128x2(
    uint8_t *out0, uint8_t *out1,
    size_t outlen,
    const uint8_t *in0, const uint8_t *in1,
    size_t inlen);

// External pack/unpack functions
extern void POLVECq2BS(uint8_t *bytes, const uint16_t data[SABER_K][SABER_N]);
extern void POLVECp2BS(uint8_t *bytes, const uint16_t data[SABER_K][SABER_N]);
extern void BS2POLq(const unsigned char *bytes, uint16_t data[SABER_N]);
extern void cbd(uint16_t *s, const unsigned char *buf);
extern void randombytes(unsigned char *x, unsigned long long xlen);
extern void poly_mul_acc(const uint16_t a[SABER_N], const uint16_t b[SABER_N], uint16_t res[SABER_N]);
extern int indcpa_kem_enc(unsigned char *message_received, unsigned char *noiseseed,
                          const unsigned char *pk, unsigned char *ciphertext);
extern int indcpa_kem_dec(const unsigned char *sk, const unsigned char *ciphertext,
                          unsigned char *message_dec);

// polyvec structure from SABER
typedef struct {
    uint16_t coeffs[SABER_N];
} poly;

typedef struct {
    poly vec[SABER_K];
} polyvec;

/*
 * GenMatrix2x - Generate 2 matrices in parallel using shake128x2
 * Based on SaberX4 GenMatrix4x
 * Uses dynamic allocation to avoid stack overflow
 */
void GenMatrix2x(
    polyvec *a0, polyvec *a1,
    const unsigned char *seed0, const unsigned char *seed1)
{
    unsigned int one_vector = 13 * SABER_N / 8;
    unsigned int byte_bank_length = SABER_K * SABER_K * one_vector;

    // Use malloc to avoid stack overflow (~7.5KB total)
    unsigned char *buf0 = malloc(byte_bank_length);
    unsigned char *buf1 = malloc(byte_bank_length);
    int i, j;

    // Batched SHAKE128 - THIS IS THE KEY OPTIMIZATION
    shake128x2(
        buf0, buf1,
        byte_bank_length,
        seed0, seed1,
        SABER_SEEDBYTES);

    // Unpack serially (same as SaberX4)
    for (i = 0; i < SABER_K; i++) {
        for (j = 0; j < SABER_K; j++) {
            BS2POLq(buf0 + (i * SABER_K + j) * one_vector, a0[i].vec[j].coeffs);
            BS2POLq(buf1 + (i * SABER_K + j) * one_vector, a1[i].vec[j].coeffs);
        }
    }

    free(buf0);
    free(buf1);
}

/*
 * GenSecret2x - Generate 2 secrets in parallel using shake128x2
 * Based on SaberX4 GenSecret4x
 * Uses dynamic allocation to avoid stack overflow
 */
void GenSecret2x(
    uint16_t r0[SABER_K][SABER_N], uint16_t r1[SABER_K][SABER_N],
    const unsigned char *seed0, const unsigned char *seed1)
{
    uint32_t i;
    int32_t buf_size = SABER_MU * SABER_N * SABER_K / 8;

    // Use malloc to avoid stack overflow (~1.5KB total)
    uint8_t *buf0 = malloc(buf_size);
    uint8_t *buf1 = malloc(buf_size);

    // Batched SHAKE128 - THIS IS THE KEY OPTIMIZATION
    shake128x2(
        buf0, buf1,
        buf_size,
        seed0, seed1,
        SABER_NOISESEEDBYTES);

    // CBD serially (same as SaberX4)
    for (i = 0; i < SABER_K; i++) {
        cbd(r0[i], buf0 + i * SABER_MU * SABER_N / 8);
        cbd(r1[i], buf1 + i * SABER_MU * SABER_N / 8);
    }

    free(buf0);
    free(buf1);
}

/*
 * MatrixVectorMul - Single matrix-vector multiplication using NEON
 * This is called SERIALLY for each keypair (like SaberX4)
 */
static void MatrixVectorMul(
    polyvec *a,
    uint16_t skpv[SABER_K][SABER_N],
    uint16_t res[SABER_K][SABER_N])
{
    int i, j, k;
    uint16_t acc[SABER_N];

    for (i = 0; i < SABER_K; i++) {
        memset(res[i], 0, sizeof(uint16_t) * SABER_N);

        for (j = 0; j < SABER_K; j++) {
            // Polynomial multiplication: acc = a[j][i] * skpv[j]
            poly_mul_acc(a[j].vec[i].coeffs, skpv[j], res[i]);
        }
    }
}

/*
 * InnerProd - Single inner product using NEON
 * This is called SERIALLY (like SaberX4)
 */
static void InnerProd(
    polyvec *b,
    uint16_t skpv[SABER_K][SABER_N],
    uint16_t res[SABER_N])
{
    int j;

    memset(res, 0, sizeof(uint16_t) * SABER_N);

    for (j = 0; j < SABER_K; j++) {
        poly_mul_acc(b[j].vec[0].coeffs, skpv[j], res);
    }
}

/*
 * TRUE batched keypair generation - processes 2 keypairs using shake128x2
 * Based on SaberX4 indcpa_kem_keypair
 * Uses dynamic allocation to avoid stack overflow (~14KB on stack otherwise)
 */
void indcpa_kem_keypair_x2(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1)
{
    // Use malloc for large buffers to avoid stack overflow
    polyvec *a0 = malloc(SABER_K * sizeof(polyvec));
    polyvec *a1 = malloc(SABER_K * sizeof(polyvec));
    uint16_t (*skpv1_0)[SABER_N] = malloc(SABER_K * sizeof(uint16_t[SABER_N]));
    uint16_t (*skpv1_1)[SABER_N] = malloc(SABER_K * sizeof(uint16_t[SABER_N]));
    uint16_t (*res)[SABER_N] = malloc(SABER_K * sizeof(uint16_t[SABER_N]));

    unsigned char seed0[SABER_SEEDBYTES], seed1[SABER_SEEDBYTES];
    unsigned char noiseseed0[SABER_COINBYTES], noiseseed1[SABER_COINBYTES];

    int i, j;

    // Generate random seeds
    extern void randombytes(unsigned char *x, unsigned long long xlen);
    randombytes(seed0, SABER_SEEDBYTES);
    randombytes(noiseseed0, SABER_COINBYTES);
    randombytes(seed1, SABER_SEEDBYTES);
    randombytes(noiseseed1, SABER_COINBYTES);

    // Hash seeds (like SaberX4 line 246)
    shake128x2(
        seed0, seed1,
        SABER_SEEDBYTES,
        seed0, seed1,
        SABER_SEEDBYTES);

    // BATCHED matrix generation
    GenMatrix2x(a0, a1, seed0, seed1);

    // BATCHED secret generation
    GenSecret2x(skpv1_0, skpv1_1, noiseseed0, noiseseed1);

    // Process each keypair SERIALLY (like SaberX4 loop at line 283)
    // Keypair 0
    MatrixVectorMul(a0, skpv1_0, res);

    // Rounding
    for (i = 0; i < SABER_K; i++) {
        for (j = 0; j < SABER_N; j++) {
            res[i][j] = (res[i][j] + (1 << (SABER_EQ - SABER_EP - 1))) >> (SABER_EQ - SABER_EP);
        }
    }

    POLVECq2BS(sk0, skpv1_0);
    POLVECp2BS(pk0, res);
    memcpy(pk0 + SABER_POLYVECCOMPRESSEDBYTES, seed0, SABER_SEEDBYTES);

    // Keypair 1
    MatrixVectorMul(a1, skpv1_1, res);

    // Rounding
    for (i = 0; i < SABER_K; i++) {
        for (j = 0; j < SABER_N; j++) {
            res[i][j] = (res[i][j] + (1 << (SABER_EQ - SABER_EP - 1))) >> (SABER_EQ - SABER_EP);
        }
    }

    POLVECq2BS(sk1, skpv1_1);
    POLVECp2BS(pk1, res);
    memcpy(pk1 + SABER_POLYVECCOMPRESSEDBYTES, seed1, SABER_SEEDBYTES);

    // Free allocated memory
    free(a0);
    free(a1);
    free(skpv1_0);
    free(skpv1_1);
    free(res);
}

/*
 * TRUE batched encryption
 */
void indcpa_kem_enc_x2(
    unsigned char *m0, unsigned char *seed0, unsigned char *pk0, unsigned char *c0,
    unsigned char *m1, unsigned char *seed1, unsigned char *pk1, unsigned char *c1)
{
    // For now, call standard version twice
    // TODO: Implement TRUE batching with GenMatrix2x and GenSecret2x
    extern int indcpa_kem_enc(unsigned char *message_received, unsigned char *noiseseed,
                              const unsigned char *pk, unsigned char *ciphertext);
    indcpa_kem_enc(m0, seed0, pk0, c0);
    indcpa_kem_enc(m1, seed1, pk1, c1);
}

/*
 * TRUE batched decryption
 */
void indcpa_kem_dec_x2(
    unsigned char *sk0, unsigned char *c0, unsigned char *m0,
    unsigned char *sk1, unsigned char *c1, unsigned char *m1)
{
    // For now, call standard version twice
    // TODO: Implement TRUE batching
    extern int indcpa_kem_dec(const unsigned char *sk, const unsigned char *ciphertext,
                              unsigned char *message_dec);
    indcpa_kem_dec(sk0, c0, m0);
    indcpa_kem_dec(sk1, c1, m1);
}
