#ifndef SABER_PARAMS_H
#define SABER_PARAMS_H

#include "../external/saber_ref/SABER_params.h"

/* SABER modulus q = 2^13 */
#define SABER_Q 8192

#define SABER_PUBLIC_KEY_BYTES   SABER_INDCPA_PUBLICKEYBYTES
#define SABER_CIPHERTEXT_BYTES   SABER_BYTES_CCA_DEC
#define SABER_SHARED_KEY_BYTES   SABER_KEYBYTES

#define MSG_BYTES                SABER_KEYBYTES
#define D_BYTES                  SABER_KEYBYTES
#define Z_BYTES                  SABER_KEYBYTES
#define NOISE_BYTES              SABER_NOISE_SEEDBYTES

#define PK_BYTES         SABER_INDCPA_PUBLICKEYBYTES
#define CT_CORE_BYTES    SABER_BYTES_CCA_DEC

/* Secret key layout (matches reference implementation):
 * sk = [s || pk || h(pk) || z]
 * - s:     SABER_INDCPA_SECRETKEYBYTES (CPA secret key)
 * - pk:    SABER_INDCPA_PUBLICKEYBYTES (public key copy)
 * - h(pk): SABER_HASHBYTES              (hash of public key)
 * - z:     SABER_KEYBYTES               (random seed for implicit rejection)
 */
#define SABER_SECRET_KEY_BYTES   (SABER_INDCPA_SECRETKEYBYTES + SABER_INDCPA_PUBLICKEYBYTES + SABER_HASHBYTES + SABER_KEYBYTES)


#endif //SABER_PARAMS_H
