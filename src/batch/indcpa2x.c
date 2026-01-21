#include <string.h>
#include <stdint.h>
#include <arm_neon.h>

#include "SABER_params.h"
#include "SABER_indcpa.h"
#include "fips202.h"
#include "fips202x2.h"
#include "cbd.h"
#include "pack_unpack.h"
#include "NTT.h"
#include "neon_poly_batch.h"

#define h1 (1 << (SABER_EQ - SABER_EP - 1))
#define h2 ((1 << (SABER_EP - 2)) - (1 << (SABER_EP - SABER_ET - 1)) + (1 << (SABER_EQ - SABER_EP - 1)))

// Assembly function declarations
extern void __asm_10_to_32(void*, const void*);
extern void __asm_13_to_32(void*, const void*);
extern void __asm_16_to_32(void*, const void*);
extern void __asm_round(uint16_t des[SABER_N], uint32_t src[SABER_N]);
extern void __asm_enc_add_msg(uint16_t cipher[SABER_N], uint32_t src[SABER_N], uint16_t msg[SABER_N], int const_h1);
extern void __asm_dec_get_msg(uint16_t msg[SABER_N], uint32_t src[SABER_N], uint16_t cipher[SABER_N], int const_h2);

/*
 * SaberX2 parallel key generation
 * Processes 2 KEM instances simultaneously using NEON for polynomial operations
 */
void indcpa_kem_keypair2x(
    uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES], uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES],
    uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES], uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES],
    const uint8_t seed_A0[SABER_SEEDBYTES], const uint8_t seed_A1[SABER_SEEDBYTES],
    const uint8_t seed_s0[SABER_NOISE_SEEDBYTES], const uint8_t seed_s1[SABER_NOISE_SEEDBYTES])
{
    // Two instances of NTT data structures
    uint32_t A_NTT0[SABER_L][SABER_L][SABER_N];
    uint32_t A_NTT1[SABER_L][SABER_L][SABER_N];

    uint32_t s_NTT0[SABER_L][SABER_N];
    uint32_t s_NTT1[SABER_L][SABER_N];

    uint32_t s_NTT_asymmetric0[SABER_L][SABER_N];
    uint32_t s_NTT_asymmetric1[SABER_L][SABER_N];

    uint16_t s0[SABER_L][SABER_N];
    uint16_t s1[SABER_L][SABER_N];

    uint16_t b0[SABER_L][SABER_N] = {0};
    uint16_t b1[SABER_L][SABER_N] = {0};

    uint8_t shake_A_buf0[SABER_L * SABER_L * SABER_POLYBYTES];
    uint8_t shake_A_buf1[SABER_L * SABER_L * SABER_POLYBYTES];

    uint8_t shake_s_buf0[SABER_L * SABER_POLYCOINBYTES];
    uint8_t shake_s_buf1[SABER_L * SABER_POLYCOINBYTES];

    // Parallel SHAKE-128 for matrix generation (2x speedup here)
    shake128x2(shake_A_buf0, shake_A_buf1, sizeof(shake_A_buf0),
               seed_A0, seed_A1, SABER_SEEDBYTES);

    // Parallel SHAKE-128 for secret generation (2x speedup here)
    shake128x2(shake_s_buf0, shake_s_buf1, sizeof(shake_s_buf0),
               seed_s0, seed_s1, SABER_NOISE_SEEDBYTES);

    // Load matrix A for both instances
    for(int i = 0; i < SABER_L; i++){
        for(int j = 0; j < SABER_L; j++){
            __asm_13_to_32(&(A_NTT0[j][i][0]), shake_A_buf0 + (i * SABER_L + j) * SABER_POLYBYTES);
            __asm_13_to_32(&(A_NTT1[j][i][0]), shake_A_buf1 + (i * SABER_L + j) * SABER_POLYBYTES);
        }
    }

    // Generate secrets via CBD for both instances
    for(int i = 0; i < SABER_L; i++){
        cbd(s0[i], shake_s_buf0 + i * SABER_POLYCOINBYTES);
        cbd(s1[i], shake_s_buf1 + i * SABER_POLYCOINBYTES);
    }

    // Batched 16->32 conversion using NEON (with sign extension)
    for(int i = 0; i < SABER_L; i++){
        poly_16_to_32_2x(s_NTT0[i], s0[i], s_NTT1[i], s1[i]);
    }

    // NTT transforms - can be done in parallel
    for(int i = 0; i < SABER_L; i++){
        NTT_heavy(&(s_NTT_asymmetric0[i][0]), &(s_NTT0[i][0]));
        NTT_heavy(&(s_NTT_asymmetric1[i][0]), &(s_NTT1[i][0]));
    }

    // Matrix NTT transforms
    for(int i = 0; i < SABER_L; i++){
        for(int j = 0; j < SABER_L; j++){
            NTT(&(A_NTT0[i][j][0]));
            NTT(&(A_NTT1[i][j][0]));
        }
    }

    // Matrix-vector multiplication in NTT domain
    for(int i = 0; i < SABER_L; i++){
        __asm_asymmetric_mul(&(A_NTT0[i][0][0]), &(s_NTT0[0][0]), &(s_NTT_asymmetric0[0][0]), constants);
        __asm_asymmetric_mul(&(A_NTT1[i][0][0]), &(s_NTT1[0][0]), &(s_NTT_asymmetric1[0][0]), constants);
    }

    // Inverse NTT
    for(int i = 0; i < SABER_L; i++){
        iNTT(&(A_NTT0[i][0][0]));
        iNTT(&(A_NTT1[i][0][0]));
    }

    // Batched rounding using NEON
    for(int i = 0; i < SABER_L; i++){
        poly_round_2x(b0[i], A_NTT0[i][0], b1[i], A_NTT1[i][0], h1, (SABER_EQ-SABER_EP));
    }

    // Pack results
    POLVECq2BS(sk0, s0);
    POLVECq2BS(sk1, s1);

    POLVECp2BS(pk0, b0);
    POLVECp2BS(pk1, b1);

    memcpy(pk0 + SABER_POLYVECCOMPRESSEDBYTES, seed_A0, SABER_SEEDBYTES);
    memcpy(pk1 + SABER_POLYVECCOMPRESSEDBYTES, seed_A1, SABER_SEEDBYTES);
}

/*
 * SaberX2 parallel encapsulation
 */
void indcpa_kem_enc2x(
    const uint8_t m0[SABER_KEYBYTES], const uint8_t seed_sp0[SABER_NOISE_SEEDBYTES],
    const uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES], uint8_t ciphertext0[SABER_BYTES_CCA_DEC],
    const uint8_t m1[SABER_KEYBYTES], const uint8_t seed_sp1[SABER_NOISE_SEEDBYTES],
    const uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES], uint8_t ciphertext1[SABER_BYTES_CCA_DEC])
{
    uint32_t A_NTT0[SABER_L][SABER_L][SABER_N];
    uint32_t A_NTT1[SABER_L][SABER_L][SABER_N];

    uint32_t s_NTT0[SABER_L][SABER_N];
    uint32_t s_NTT1[SABER_L][SABER_N];

    uint32_t s_NTT_asymmetric0[SABER_L][SABER_N];
    uint32_t s_NTT_asymmetric1[SABER_L][SABER_N];

    uint32_t b_NTT0[SABER_L][SABER_N];
    uint32_t b_NTT1[SABER_L][SABER_N];

    uint16_t sp0[SABER_L][SABER_N];
    uint16_t sp1[SABER_L][SABER_N];

    uint16_t bp0[SABER_L][SABER_N] = {0};
    uint16_t bp1[SABER_L][SABER_N] = {0};

    uint16_t vp0[SABER_N] = {0};
    uint16_t vp1[SABER_N] = {0};

    uint16_t mp0[SABER_N];
    uint16_t mp1[SABER_N];

    uint16_t b0[SABER_L][SABER_N];
    uint16_t b1[SABER_L][SABER_N];

    const uint8_t *seed_A0 = pk0 + SABER_POLYVECCOMPRESSEDBYTES;
    const uint8_t *seed_A1 = pk1 + SABER_POLYVECCOMPRESSEDBYTES;

    uint8_t shake_A_buf0[SABER_L * SABER_L * SABER_POLYBYTES];
    uint8_t shake_A_buf1[SABER_L * SABER_L * SABER_POLYBYTES];

    uint8_t shake_s_buf0[SABER_L * SABER_POLYCOINBYTES];
    uint8_t shake_s_buf1[SABER_L * SABER_POLYCOINBYTES];

    // Parallel SHAKE-128
    shake128x2(shake_A_buf0, shake_A_buf1, sizeof(shake_A_buf0),
               seed_A0, seed_A1, SABER_SEEDBYTES);

    shake128x2(shake_s_buf0, shake_s_buf1, sizeof(shake_s_buf0),
               seed_sp0, seed_sp1, SABER_NOISE_SEEDBYTES);

    // Load matrix A
    for(int i = 0; i < SABER_L; i++){
        for(int j = 0; j < SABER_L; j++){
            __asm_13_to_32(&(A_NTT0[i][j][0]), shake_A_buf0 + (i * SABER_L + j) * SABER_POLYBYTES);
            __asm_13_to_32(&(A_NTT1[i][j][0]), shake_A_buf1 + (i * SABER_L + j) * SABER_POLYBYTES);
        }
    }

    // Generate sp via CBD
    for(int i = 0; i < SABER_L; i++){
        cbd(sp0[i], shake_s_buf0 + i * SABER_POLYCOINBYTES);
        cbd(sp1[i], shake_s_buf1 + i * SABER_POLYCOINBYTES);
    }

    // Batched 16->32 conversion using NEON (with sign extension)
    for(int i = 0; i < SABER_L; i++){
        poly_16_to_32_2x(s_NTT0[i], sp0[i], s_NTT1[i], sp1[i]);
    }

    // NTT transforms
    for(int i = 0; i < SABER_L; i++){
        NTT_heavy(&(s_NTT_asymmetric0[i][0]), &(s_NTT0[i][0]));
        NTT_heavy(&(s_NTT_asymmetric1[i][0]), &(s_NTT1[i][0]));
    }

    for(int i = 0; i < SABER_L; i++){
        for(int j = 0; j < SABER_L; j++){
            NTT(&(A_NTT0[i][j][0]));
            NTT(&(A_NTT1[i][j][0]));
        }
    }

    // Matrix-vector multiplication
    for(int i = 0; i < SABER_L; i++){
        __asm_asymmetric_mul(&(A_NTT0[i][0][0]), &(s_NTT0[0][0]), &(s_NTT_asymmetric0[0][0]), constants);
        __asm_asymmetric_mul(&(A_NTT1[i][0][0]), &(s_NTT1[0][0]), &(s_NTT_asymmetric1[0][0]), constants);
    }

    // Inverse NTT
    for(int i = 0; i < SABER_L; i++){
        iNTT(&(A_NTT0[i][0][0]));
        iNTT(&(A_NTT1[i][0][0]));
    }

    // Batched rounding using NEON
    for(int i = 0; i < SABER_L; i++){
        poly_round_2x(bp0[i], A_NTT0[i][0], bp1[i], A_NTT1[i][0], h1, (SABER_EQ-SABER_EP));
    }

    // Unpack public key b
    BS2POLVECp(pk0, b0);
    BS2POLVECp(pk1, b1);

    BS2POLmsg(m0, mp0);
    BS2POLmsg(m1, mp1);

    // Inner product for v'
    for(int i = 0; i < SABER_L; i++){
        __asm_16_to_32(&(b_NTT0[i][0]), &(b0[i][0]));
        __asm_16_to_32(&(b_NTT1[i][0]), &(b1[i][0]));
    }

    for(int i = 0; i < SABER_L; i++){
        NTT(&(b_NTT0[i][0]));
        NTT(&(b_NTT1[i][0]));
    }

    __asm_asymmetric_mul(&(b_NTT0[0][0]), &(s_NTT0[0][0]), &(s_NTT_asymmetric0[0][0]), constants);
    __asm_asymmetric_mul(&(b_NTT1[0][0]), &(s_NTT1[0][0]), &(s_NTT_asymmetric1[0][0]), constants);

    iNTT(&(b_NTT0[0][0]));
    iNTT(&(b_NTT1[0][0]));

    __asm_enc_add_msg(vp0, b_NTT0[0], mp0, h1);
    __asm_enc_add_msg(vp1, b_NTT1[0], mp1, h1);

    // Pack ciphertext
    POLVECp2BS(ciphertext0, bp0);
    POLVECp2BS(ciphertext1, bp1);

    POLT2BS(ciphertext0 + SABER_POLYVECCOMPRESSEDBYTES, vp0);
    POLT2BS(ciphertext1 + SABER_POLYVECCOMPRESSEDBYTES, vp1);
}

/*
 * SaberX2 parallel decapsulation
 */
void indcpa_kem_dec2x(
    const uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES], const uint8_t ciphertext0[SABER_BYTES_CCA_DEC], uint8_t m0[SABER_KEYBYTES],
    const uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES], const uint8_t ciphertext1[SABER_BYTES_CCA_DEC], uint8_t m1[SABER_KEYBYTES])
{
    uint32_t b_NTT0[SABER_L][SABER_N];
    uint32_t b_NTT1[SABER_L][SABER_N];

    uint32_t s_NTT0[SABER_L][SABER_N];
    uint32_t s_NTT1[SABER_L][SABER_N];

    uint32_t s_NTT_asymmetric0[SABER_L][SABER_N];
    uint32_t s_NTT_asymmetric1[SABER_L][SABER_N];

    uint16_t v0[SABER_N] = {0};
    uint16_t v1[SABER_N] = {0};

    uint16_t cm0[SABER_N];
    uint16_t cm1[SABER_N];

    BS2POLT(ciphertext0 + SABER_POLYVECCOMPRESSEDBYTES, cm0);
    BS2POLT(ciphertext1 + SABER_POLYVECCOMPRESSEDBYTES, cm1);

    // Load secret key
    for(int i = 0; i < SABER_L; i++){
        __asm_13_to_32(&(s_NTT0[i][0]), sk0 + i * SABER_POLYBYTES);
        __asm_13_to_32(&(s_NTT1[i][0]), sk1 + i * SABER_POLYBYTES);
    }

    // Load ciphertext
    for(int i = 0; i < SABER_L; i++){
        __asm_10_to_32(&(b_NTT0[i][0]), ciphertext0 + i * (SABER_EP * SABER_N / 8));
        __asm_10_to_32(&(b_NTT1[i][0]), ciphertext1 + i * (SABER_EP * SABER_N / 8));
    }

    // NTT transforms
    for(int i = 0; i < SABER_L; i++){
        NTT_heavy(&(s_NTT_asymmetric0[i][0]), &(s_NTT0[i][0]));
        NTT_heavy(&(s_NTT_asymmetric1[i][0]), &(s_NTT1[i][0]));
    }

    for(int i = 0; i < SABER_L; i++){
        NTT(&(b_NTT0[i][0]));
        NTT(&(b_NTT1[i][0]));
    }

    // Inner product
    __asm_asymmetric_mul(&(b_NTT0[0][0]), &(s_NTT0[0][0]), &(s_NTT_asymmetric0[0][0]), constants);
    __asm_asymmetric_mul(&(b_NTT1[0][0]), &(s_NTT1[0][0]), &(s_NTT_asymmetric1[0][0]), constants);

    iNTT(&(b_NTT0[0][0]));
    iNTT(&(b_NTT1[0][0]));

    __asm_dec_get_msg(v0, b_NTT0[0], cm0, h2);
    __asm_dec_get_msg(v1, b_NTT1[0], cm1, h2);

    POLmsg2BS(m0, v0);
    POLmsg2BS(m1, v1);
}
