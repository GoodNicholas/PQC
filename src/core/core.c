/**
 * @file core.c
 * @brief Core CPA operations for SABER_GOST
 *
 * Implements CPA-secure Saber operations:
 * - Key generation
 * - Encryption
 * - Decryption
 *
 * These operations are used by the FO-transformation in kem.c to build
 * a CCA-secure KEM.
 */

#include "../include/core.h"
#include "../include/rng.h"
#include "../include/hash.h"
#include "../include/poly.h"
#include "../include/config.h"
#include "../include/params.h"
#include "../external/saber_ref/SABER_indcpa.h"
#include <string.h>

/* ========================================================================
 * ГЕНЕРАЦИЯ КЛЮЧЕЙ
 * ======================================================================== */

/**
 * SaberCore_KeyGen - генерация пары ключей CPA-Saber
 *
 * Структура ключей:
 * - pk: [seed_A || b] где seed_A - 32 байта, b - SABER_POLYVECCOMPRESSEDBYTES
 * - sk: [s] где s - SABER_INDCPA_SECRETKEYBYTES
 *
 * Алгоритм:
 * 1. Генерация CPA ключей через indcpa_kem_keypair()
 *
 * Примечание: z и другие элементы секретного ключа добавляются в Saber_KeyGen
 */
void SaberCore_KeyGen(uint8_t *pk, uint8_t *sk)
{
    /* Генерация CPA ключей: pk = [seed_A || b], sk = s */
    indcpa_kem_keypair(pk, sk);
}

/* ========================================================================
 * ШИФРОВАНИЕ
 * ======================================================================== */

/**
 * SaberCore_Encrypt - CPA-шифрование сообщения
 *
 * ДЕТЕРМИНИРОВАННАЯ функция: одинаковые (pk, m, coins) → одинаковый ct.
 *
 * Алгоритм (реализован в indcpa_kem_enc):
 * 1. Распаковать pk: (seed_A || b) = pk
 * 2. A ← gen_matrix_A(seed_A)
 * 3. s' ← CBD(coins)
 * 4. b' = A * s' + h
 * 5. v = <b, s'> + h'
 * 6. cm = v + encode(m)
 * 7. ct = (b' || cm)
 */
void SaberCore_Encrypt(const uint8_t *pk,
                       const uint8_t *m,
                       const uint8_t *coins,
                       uint8_t *ct)
{
    /* Прямой вызов CPA-шифрования из Reference_Implementation */
    indcpa_kem_enc(m, coins, pk, ct);
}

/* ========================================================================
 * ДЕШИФРОВАНИЕ
 * ======================================================================== */

/**
 * SaberCore_Decrypt - CPA-дешифрование шифротекста
 *
 * Алгоритм (реализован в indcpa_kem_dec):
 * 1. Распаковать ct: (b' || cm) = ct
 * 2. Распаковать sk: s = sk[0:SABER_INDCPA_SECRETKEYBYTES]
 * 3. v' = <b', s>
 * 4. m = decode(cm - v')
 */
void SaberCore_Decrypt(const uint8_t *sk,
                       const uint8_t *ct,
                       uint8_t *m)
{
    /* Прямой вызов CPA-дешифрования из Reference_Implementation */
    indcpa_kem_dec(sk, ct, m);
}
