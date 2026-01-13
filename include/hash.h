#ifndef SABER_HASH_H
#define SABER_HASH_H

#include <stdint.h>
#include <stddef.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hash.h
 * @brief Интерфейс хэш-модуля для SABER_GOST
 *
 * Поддерживает различные хэш-функции в зависимости от конфигурации:
 * - SHA-3 (DEFAULT, FAST, TEST)
 * - Streebog/ГОСТ Р 34.11-2012 (GOST, GOST_FAST)
 */

/* ========================================================================
 * ОСНОВНЫЕ ХЭШ-ФУНКЦИИ
 * ======================================================================== */

/**
 * @brief H1 - хэш-функция для вычисления подтверждающего хэша 'd'
 *
 * Используется в FO-трансформации: d = H1(m || c_core)
 *
 * @param[out] digest  Выходной буфер размера SABER_HASHBYTES (32 байта)
 * @param[in]  in1     Первый входной блок данных
 * @param[in]  len1    Длина первого блока
 * @param[in]  in2     Второй входной блок данных
 * @param[in]  len2    Длина второго блока
 */
void H1(uint8_t *digest,
        const uint8_t *in1, size_t len1,
        const uint8_t *in2, size_t len2);

/**
 * @brief H2 - хэш-функция для вычисления общего ключа
 *
 * Используется в FO-трансформации: shared_key = H2(m || c_core)
 *
 * @param[out] key     Выходной буфер размера SABER_KEYBYTES (32 байта)
 * @param[in]  in1     Первый входной блок данных
 * @param[in]  len1    Длина первого блока
 * @param[in]  in2     Второй входной блок данных
 * @param[in]  len2    Длина второго блока
 */
void H2(uint8_t *key,
        const uint8_t *in1, size_t len1,
        const uint8_t *in2, size_t len2);

/**
 * @brief KDF_fail - функция деривации ключа при неудачной декапсуляции
 *
 * Используется для implicit rejection: если декапсуляция не прошла валидацию,
 * вместо m используется случайное значение z.
 *
 * @param[out] key     Выходной буфер размера SABER_KEYBYTES (32 байта)
 * @param[in]  z       Случайное значение z (Z_BYTES = 32 байта)
 * @param[in]  ct      Шифротекст
 * @param[in]  ct_len  Длина шифротекста
 */
void KDF_fail(uint8_t *key,
              const uint8_t *z,
              const uint8_t *ct,
              size_t ct_len);

/* ========================================================================
 * XOF (EXTENDABLE OUTPUT FUNCTION)
 * ======================================================================== */

/**
 * @brief XOF - расширяемая выходная функция (XOF)
 *
 * Генерирует произвольное количество псевдослучайных байт из seed.
 * - SHA-3 конфигурация: используется SHAKE128
 * - GOST конфигурация: используется Streebog-256 в counter mode
 *
 * @param[out] out     Выходной буфер
 * @param[in]  outlen  Требуемая длина выхода (в байтах)
 * @param[in]  seed    Входной seed
 * @param[in]  seedlen Длина seed
 */
void XOF(uint8_t *out, size_t outlen,
         const uint8_t *seed, size_t seedlen);

/**
 * @brief XOF_batch4 - параллельная генерация 4 XOF одновременно (NEON)
 *
 * Доступно только в FAST конфигурации с SHAKE×4 NEON.
 * Генерирует 4 независимых XOF выхода параллельно для ускорения.
 *
 * @param[out] out0    Первый выходной буфер
 * @param[out] out1    Второй выходной буфер
 * @param[out] out2    Третий выходной буфер
 * @param[out] out3    Четвертый выходной буфер
 * @param[in]  outlen  Требуемая длина каждого выхода
 * @param[in]  seed0   Первый seed
 * @param[in]  seed1   Второй seed
 * @param[in]  seed2   Третий seed
 * @param[in]  seed3   Четвертый seed
 * @param[in]  seedlen Длина каждого seed
 */
#ifdef USE_SHAKE4X_NEON
void XOF_batch4(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                size_t outlen,
                const uint8_t *seed0, const uint8_t *seed1,
                const uint8_t *seed2, const uint8_t *seed3,
                size_t seedlen);
#endif

/* ========================================================================
 * ГЕНЕРАЦИЯ МАТРИЦЫ A
 * ======================================================================== */

/**
 * @brief gen_matrix_A - генерация публичной матрицы A из seed
 *
 * Генерирует матрицу A размером SABER_L × SABER_L × SABER_N из seed.
 * Матрица A является публичным параметром, используемым в операциях шифрования.
 *
 * Оптимизации:
 * - DEFAULT/GOST: последовательная генерация через XOF
 * - FAST: параллельная генерация через SHAKE×4 NEON (ускорение ~3.2×)
 *
 * @param[out] A      Выходная матрица [SABER_L][SABER_L][SABER_N]
 * @param[in]  seed   Входной seed размера SABER_SEEDBYTES (32 байта)
 */
void gen_matrix_A(uint16_t A[SABER_L][SABER_L][SABER_N],
                  const uint8_t seed[SABER_SEEDBYTES]);

#ifdef __cplusplus
}
#endif

#endif /* SABER_HASH_H */
