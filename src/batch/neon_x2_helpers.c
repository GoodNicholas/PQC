/**
 * Helper functions for SaberX2 NEON
 */

#include <arm_neon.h>
#include <string.h>
#include "fips202.h"
#include "params.h"

/**
 * SHAKE128 with 2 different nonces in parallel
 * This simulates parallel SHAKE for 2 operations
 */
void shake128_absorb_twice(
    uint8_t *out0, uint8_t *out1, size_t outlen,
    const uint8_t *seed, size_t seedlen,
    uint8_t nonce0, uint8_t nonce1)
{
    uint8_t buf0[seedlen + 1];
    uint8_t buf1[seedlen + 1];

    memcpy(buf0, seed, seedlen);
    memcpy(buf1, seed, seedlen);
    buf0[seedlen] = nonce0;
    buf1[seedlen] = nonce1;

    shake128(out0, outlen, buf0, seedlen + 1);
    shake128(out1, outlen, buf1, seedlen + 1);
}

/**
 * Polynomial multiplication with accumulation
 * This should use Toom-Cook or NTT in real implementation
 */
void poly_mul_acc(poly *r, const poly *a, const poly *b) {
    uint32_t temp[2*SABER_N] = {0};

    // Schoolbook multiplication (simplified)
    for (int i = 0; i < SABER_N; i++) {
        for (int j = 0; j < SABER_N; j++) {
            temp[i+j] += (uint32_t)a->coeffs[i] * b->coeffs[j];
        }
    }

    // Reduction modulo x^256 + 1
    for (int i = 0; i < SABER_N; i++) {
        r->coeffs[i] += temp[i] - temp[i + SABER_N];
    }
}

/**
 * Constant-time conditional move
 */
void cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b) {
    b = -b;
    for (size_t i = 0; i < len; i++) {
        r[i] ^= b & (r[i] ^ x[i]);
    }
}

/**
 * Constant-time comparison
 */
uint8_t verify(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t r = 0;
    for (size_t i = 0; i < len; i++) {
        r |= a[i] ^ b[i];
    }
    return (uint8_t)(-((r | -r) >> 7)) + 1;
}