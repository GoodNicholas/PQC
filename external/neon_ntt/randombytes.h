#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * randombytes - generate random bytes
 * Implemented in SABER_GOST/src/kem/core_neon_ntt_wrapper.c
 */
void randombytes(unsigned char *x, unsigned long long xlen);

#ifdef __cplusplus
}
#endif

#endif /* RANDOMBYTES_H */
