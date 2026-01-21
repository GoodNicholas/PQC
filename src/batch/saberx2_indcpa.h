/*
 * SaberX2 IND-CPA operations
 */

#ifndef SABERX2_INDCPA_H
#define SABERX2_INDCPA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void indcpa_kem_keypair_x2(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1);

void indcpa_kem_enc_x2(
    unsigned char *m0, unsigned char *m1,
    unsigned char *seed_sp0, unsigned char *seed_sp1,
    unsigned char *pk0, unsigned char *c0,
    unsigned char *pk1, unsigned char *c1);

void indcpa_kem_dec_x2(
    unsigned char *sk0, unsigned char *c0, unsigned char *m0,
    unsigned char *sk1, unsigned char *c1, unsigned char *m1);

#ifdef __cplusplus
}
#endif

#endif // SABERX2_INDCPA_H
