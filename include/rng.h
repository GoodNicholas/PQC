#ifndef SABER_RNG_H
#define SABER_RNG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file rng.h
 * @brief Интерфейс модуля генерации случайных чисел (RNG) для SABER_GOST
 *
 * Поддерживает различные RNG в зависимости от конфигурации:
 * - System RNG (DEFAULT) - использует arc4random_buf
 * - ChaCha20 NEON (FAST) - детерминированный DRBG с NEON оптимизациями
 * - Kuznyechik CTR (GOST, GOST_FAST) - ГОСТ Р 34.12-2015 в режиме CTR
 * - CTR_DRBG (TEST) - NIST SP 800-90A для воспроизводимых тестов
 */

/* ========================================================================
 * ОСНОВНЫЕ ФУНКЦИИ
 * ======================================================================== */

/**
 * @brief random_bytes - генерация случайных байт
 *
 * Генерирует len случайных байт и записывает в buf.
 *
 * Поведение в зависимости от конфигурации:
 * - System RNG: использует системный источник энтропии (arc4random_buf)
 * - ChaCha20/CTR_DRBG/GOST CTR: использует детерминированный DRBG
 *   (требуется предварительная инициализация через rng_init)
 *
 * @param[out] buf  Выходной буфер для случайных байт
 * @param[in]  len  Количество байт для генерации
 */
void random_bytes(uint8_t *buf, size_t len);

/**
 * @brief rng_init - инициализация детерминированного RNG
 *
 * Инициализирует DRBG с заданным seed. Используется для детерминированных
 * конфигураций (ChaCha20, CTR_DRBG, GOST CTR).
 *
 * Схема работы (двухуровневая):
 * 1. System entropy → seed (из системного источника)
 * 2. DRBG(seed) → random_bytes() (детерминированная генерация)
 *
 * Для System RNG эта функция ничего не делает.
 *
 * @param[in] seed     Seed для инициализации DRBG
 * @param[in] seedlen  Длина seed (обычно 32 байта)
 */
void rng_init(const uint8_t *seed, size_t seedlen);

#ifdef __cplusplus
}
#endif

#endif /* SABER_RNG_H */
