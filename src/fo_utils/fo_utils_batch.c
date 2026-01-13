/**
 * @file fo_utils_batch.c
 * @brief GOST implementation of FO utilities for SABER_GOST (deterministic version)
 *
 * Implements Fujisaki-Okamoto transformation utilities using Streebog-256.
 *
 * Note: Batch optimization removed to ensure determinism.
 * Original batch code had a stateful pool that broke determinism requirement.
 */

#include "../../include/fo_utils.h"
#include "../../include/hash.h"
#include "../../include/config.h"
#include "../../include/params.h"
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * ГЕНЕРАЦИЯ МОНЕТ
 * ======================================================================== */

/**
 * generate_coins - детерминированная генерация монет через Streebog-256
 *
 * Алгоритм (из плана):
 * 1. seed = Streebog-256(m)
 * 2. coins = Streebog-256(seed || pk)
 *
 * КРИТИЧЕСКИ ВАЖНО: функция детерминированная!
 * Одинаковые (m, pk) → одинаковые coins.
 */
void generate_coins(uint8_t *coins,
                    const uint8_t *m,
                    const uint8_t *pk)
{
    /* Шаг 1: seed = Streebog-256(m) */
    uint8_t seed[SABER_HASHBYTES];  /* 32 bytes */
    H2(seed, m, MSG_BYTES, NULL, 0);

    /* Шаг 2: coins = Streebog-256(seed || pk) */
    size_t pk_len = SABER_INDCPA_PUBLICKEYBYTES;
    H2(coins, seed, SABER_HASHBYTES, pk, pk_len);
}

/* ========================================================================
 * ВЫЧИСЛЕНИЕ ПОДТВЕРЖДАЮЩЕГО ХЭША И ОБЩЕГО КЛЮЧА
 * ======================================================================== */

/**
 * compute_d - вычисление подтверждающего хэша 'd'
 *
 * d = H1(m || ct)
 */
void compute_d(const uint8_t *m,
               const uint8_t *ct,
               uint8_t *d)
{
    H1(d, m, MSG_BYTES, ct, SABER_CIPHERTEXT_BYTES);
}

/**
 * compute_shared - вычисление общего ключа
 *
 * shared_key = H2(m || ct)
 */
void compute_shared(const uint8_t *m,
                    const uint8_t *ct,
                    uint8_t *shared_key)
{
    H2(shared_key, m, MSG_BYTES, ct, SABER_CIPHERTEXT_BYTES);
}
