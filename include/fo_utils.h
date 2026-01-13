#ifndef SABER_FO_UTILS_H
#define SABER_FO_UTILS_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file fo_utils.h
 * @brief Интерфейс модуля FO-утилит для SABER_GOST
 *
 * Содержит вспомогательные функции для FO-трансформации (Fujisaki-Okamoto).
 * Поддерживает различные оптимизации в зависимости от конфигурации:
 * - Reference (DEFAULT, TEST) - базовая реализация через SHA-3
 * - ChaCha20 NEON (FAST) - оптимизированная генерация монет через ChaCha20
 * - Batch (GOST, GOST_FAST) - батчевая предгенерация для серверных приложений
 */

/* ========================================================================
 * ГЕНЕРАЦИЯ МОНЕТ
 * ======================================================================== */

/**
 * @brief generate_coins - детерминированная генерация монет для шифрования
 *
 * Генерирует случайные монеты (noise seed) для CPA-шифрования.
 * КРИТИЧЕСКИ ВАЖНО: функция должна быть ДЕТЕРМИНИРОВАННОЙ для корректности
 * FO-трансформации. Одинаковые (m, pk) → одинаковые coins.
 *
 * Алгоритм (DEFAULT, TEST):
 * 1. seed = H(m)          // SHA-3-256
 * 2. coins = H(seed || pk) // SHA-3-256
 *
 * Алгоритм (FAST):
 * 1. seed = SHA3(m)
 * 2. nonce = SHA3(pk)[0:8]
 * 3. coins = ChaCha20(seed, nonce)  // NEON-ускорение ~3.3×
 *
 * Алгоритм (GOST, GOST_FAST):
 * 1. seed = Streebog-256(m)
 * 2. coins = Streebog-256(seed || pk)
 * 3. Batch-оптимизация: предгенерация пула монет (ускорение ~1.6×)
 *
 * @param[out] coins  Выходной буфер для монет размера NOISE_BYTES (32 байта)
 * @param[in]  m      Сообщение размера MSG_BYTES (32 байта)
 * @param[in]  pk     Публичный ключ размера SABER_INDCPA_PUBLICKEYBYTES
 */
void generate_coins(uint8_t *coins,
                    const uint8_t *m,
                    const uint8_t *pk);

/* ========================================================================
 * ВЫЧИСЛЕНИЕ ПОДТВЕРЖДАЮЩЕГО ХЭША И ОБЩЕГО КЛЮЧА
 * ======================================================================== */

/**
 * @brief compute_d - вычисление подтверждающего хэша 'd'
 *
 * Вычисляет d = H1(m || ct) для проверки корректности декапсуляции.
 * В шифротексте хранится: ct = c_core || d
 *
 * @param[in]  m   Сообщение размера MSG_BYTES (32 байта)
 * @param[in]  ct  Шифротекст размера CT_CORE_BYTES
 * @param[out] d   Подтверждающий хэш размера D_BYTES (32 байта)
 */
void compute_d(const uint8_t *m,
               const uint8_t *ct,
               uint8_t *d);

/**
 * @brief compute_shared - вычисление общего ключа
 *
 * Вычисляет shared_key = H2(m || ct).
 * Используется как в Encaps, так и в Decaps для получения итогового ключа.
 *
 * @param[in]  m          Сообщение размера MSG_BYTES (32 байта)
 * @param[in]  ct         Шифротекст размера CT_CORE_BYTES
 * @param[out] shared_key Общий ключ размера SABER_KEYBYTES (32 байта)
 */
void compute_shared(const uint8_t *m,
                    const uint8_t *ct,
                    uint8_t *shared_key);

#ifdef __cplusplus
}
#endif

#endif /* SABER_FO_UTILS_H */
