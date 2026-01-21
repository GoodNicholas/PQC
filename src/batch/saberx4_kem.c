/*
 * SaberX4 KEM wrapper - batch4 API
 * Provides public interface for 4x parallel key generation
 */

#include <stdint.h>
#include "SABER_params.h"

// Forward declaration
extern void indcpa_kem_keypair_x4(
    uint8_t *pk0, uint8_t *sk0,
    uint8_t *pk1, uint8_t *sk1,
    uint8_t *pk2, uint8_t *sk2,
    uint8_t *pk3, uint8_t *sk3);

/*
 * batch4_keygen: Generate 4 keypairs simultaneously
 *
 * This provides ~1.1x throughput improvement over sequential generation
 * by batching SHAKE128 operations (2x shake128x2 calls).
 *
 * Poly operations remain sequential.
 */
int saber_batch4_keygen(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t pk2[SABER_PUBLICKEYBYTES],
    uint8_t pk3[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES],
    uint8_t sk2[SABER_SECRETKEYBYTES],
    uint8_t sk3[SABER_SECRETKEYBYTES])
{
    indcpa_kem_keypair_x4(pk0, sk0, pk1, sk1, pk2, sk2, pk3, sk3);
    return 0;
}
