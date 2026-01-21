/*
 * Simplified fips202x2 - just calls sha3 twice sequentially
 * TODO: Replace with true NEON batched version later
 */

#include "../../include/batch/fips202x2_neon.h"
#include "../../external/saber_ref/fips202.h"

void sha3_256x2(uint8_t h1[32],
                uint8_t h2[32],
                const uint8_t *in1,
                const uint8_t *in2,
                size_t inlen)
{
    sha3_256(h1, in1, inlen);
    sha3_256(h2, in2, inlen);
}

void sha3_512x2(uint8_t h1[64],
                uint8_t h2[64],
                const uint8_t *in1,
                const uint8_t *in2,
                size_t inlen)
{
    sha3_512(h1, in1, inlen);
    sha3_512(h2, in2, inlen);
}

void shake128x2(uint8_t *out0,
                uint8_t *out1,
                size_t outlen,
                const uint8_t *in0,
                const uint8_t *in1,
                size_t inlen)
{
    shake128(out0, outlen, in0, inlen);
    shake128(out1, outlen, in1, inlen);
}

// shake256x2 not needed for SABER
