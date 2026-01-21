/*
 * SaberX2 IND-CPA - SIMPLIFIED VERSION
 * Just calls standard SABER functions twice
 * This is a quick implementation to get SaberX2 working
 * TODO: Optimize with real NEON batching later
 */

#include <string.h>
#include "saberx2_indcpa.h"
#include "../../external/saber_ref/SABER_indcpa.h"
#include "../../external/saber_ref/SABER_params.h"

void indcpa_kem_keypair_x2(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1)
{
    // Simple approach: just call twice
    indcpa_kem_keypair(pk0, sk0);
    indcpa_kem_keypair(pk1, sk1);
}

void indcpa_kem_enc_x2(
    unsigned char *m0, unsigned char *m1,
    unsigned char *seed_sp0, unsigned char *seed_sp1,
    unsigned char *pk0, unsigned char *c0,
    unsigned char *pk1, unsigned char *c1)
{
    // Simple approach: just call twice
    indcpa_kem_enc(m0, seed_sp0, pk0, c0);
    indcpa_kem_enc(m1, seed_sp1, pk1, c1);
}

void indcpa_kem_dec_x2(
    unsigned char *sk0, unsigned char *c0, unsigned char *m0,
    unsigned char *sk1, unsigned char *c1, unsigned char *m1)
{
    // Simple approach: just call twice
    indcpa_kem_dec(sk0, c0, m0);
    indcpa_kem_dec(sk1, c1, m1);
}
