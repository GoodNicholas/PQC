#ifndef SABER_CORE_H
#define SABER_CORE_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file core.h
 * @brief Интерфейс core операций CPA-безопасной схемы Saber
 *
 * Содержит базовые криптографические операции:
 * - Генерация ключей (CPA)
 * - Шифрование (CPA)
 * - Дешифрование (CPA)
 *
 * Эти операции используются в FO-трансформации в kem.c для построения
 * CCA-безопасной схемы инкапсуляции ключей (KEM).
 */

/* ========================================================================
 * ГЕНЕРАЦИЯ КЛЮЧЕЙ
 * ======================================================================== */

/**
 * @brief SaberCore_KeyGen - генерация пары ключей CPA-Saber
 *
 * Генерирует пару ключей для CPA-безопасной схемы Saber:
 * 1. seed_A ← random(SABER_SEEDBYTES)
 * 2. A ← gen_matrix_A(seed_A)
 * 3. seed_s ← random(SABER_NOISE_SEEDBYTES)
 * 4. s ← CBD(seed_s)  // Centered Binomial Distribution
 * 5. b = A^T * s + h  // h - rounding для сжатия
 * 6. pk = (seed_A || b), sk = s
 * 7. z ← random(Z_BYTES)  // для implicit rejection в FO
 *
 * @param[out] pk  Публичный ключ размера SABER_INDCPA_PUBLICKEYBYTES (992 байта)
 * @param[out] sk  Секретный ключ размера SK_CORE_BYTES (1248+32 = 1280 байт)
 *                 Структура: sk = (s || z)
 */
void SaberCore_KeyGen(uint8_t *pk, uint8_t *sk);

/* ========================================================================
 * ШИФРОВАНИЕ
 * ======================================================================== */

/**
 * @brief SaberCore_Encrypt - CPA-шифрование сообщения
 *
 * Шифрует сообщение m с использованием публичного ключа pk и монет coins.
 * ВАЖНО: функция ДЕТЕРМИНИРОВАННАЯ - одинаковые (pk, m, coins) → одинаковый ct.
 *
 * Алгоритм:
 * 1. Распаковать pk: (seed_A || b) = pk
 * 2. A ← gen_matrix_A(seed_A)
 * 3. s' ← CBD(coins)  // случайный вектор из монет
 * 4. b' = A * s' + h
 * 5. v = <b, s'> + h'
 * 6. cm = v + encode(m)  // кодирование сообщения
 * 7. ct = (b' || cm)
 *
 * @param[in]  pk    Публичный ключ размера SABER_INDCPA_PUBLICKEYBYTES
 * @param[in]  m     Сообщение размера MSG_BYTES (32 байта)
 * @param[in]  coins Монеты размера NOISE_BYTES (32 байта)
 * @param[out] ct    Шифротекст размера CT_CORE_BYTES (1088 байт)
 */
void SaberCore_Encrypt(const uint8_t *pk,
                       const uint8_t *m,
                       const uint8_t *coins,
                       uint8_t *ct);

/* ========================================================================
 * ДЕШИФРОВАНИЕ
 * ======================================================================== */

/**
 * @brief SaberCore_Decrypt - CPA-дешифрование шифротекста
 *
 * Дешифрует шифротекст ct с использованием секретного ключа sk.
 *
 * Алгоритм:
 * 1. Распаковать ct: (b' || cm) = ct
 * 2. Распаковать sk: s = sk[0:SK_BYTES]
 * 3. v' = <b', s>
 * 4. m = decode(cm - v')  // декодирование сообщения
 *
 * @param[in]  sk  Секретный ключ размера SK_CORE_BYTES
 * @param[in]  ct  Шифротекст размера CT_CORE_BYTES
 * @param[out] m   Восстановленное сообщение размера MSG_BYTES (32 байта)
 */
void SaberCore_Decrypt(const uint8_t *sk,
                       const uint8_t *ct,
                       uint8_t *m);

#ifdef __cplusplus
}
#endif

#endif /* SABER_CORE_H */
