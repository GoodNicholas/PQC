/**
 * @file hash_sha3.c
 * @brief SHA-3 implementation of hash module for SABER_GOST
 *
 * Uses SHA-3-256 and SHAKE128 from Reference_Implementation (fips202.h).
 * This is the default hash implementation for DEFAULT, FAST, and TEST configurations.
 */

#include "../../include/hash.h"
#include "../../include/config.h"
#include "../../include/params.h"
#include "../../../SABER/Reference_Implementation_KEM/fips202.h"
#include "../../../SABER/Reference_Implementation_KEM/poly.h"
#include "../../../SABER/Reference_Implementation_KEM/pack_unpack.h"
#include <stdlib.h>
#include <string.h>

/* Keccak×3 NEON optimization - currently disabled, using reference GenMatrix */
/*
#if defined(USE_SHAKE4X_NEON) && defined(__ARM_NEON) && (SABER_L == 3)
#include "keccak_times3_neon.h"
#include "../../../SABER/Reference_Implementation_KEM/pack_unpack.h"
#endif
*/

/* ========================================================================
 * ОСНОВНЫЕ ХЭШ-ФУНКЦИИ
 * ======================================================================== */

/**
 * H1 - SHA-3-256(in1 || in2)
 */
void H1(uint8_t *digest,
        const uint8_t *in1, size_t len1,
        const uint8_t *in2, size_t len2)
{
    size_t total = len1 + len2;
    uint8_t *tmp = malloc(total);
    if (!tmp) {
        /* Ошибка выделения памяти - критическая ситуация */
        abort();
    }

    memcpy(tmp, in1, len1);
    memcpy(tmp + len1, in2, len2);

    sha3_256(digest, tmp, total);

    free(tmp);
}

/**
 * H2 - SHA-3-256(in1 || in2)
 */
void H2(uint8_t *key,
        const uint8_t *in1, size_t len1,
        const uint8_t *in2, size_t len2)
{
    size_t total = len1 + len2;
    uint8_t *tmp = malloc(total);
    if (!tmp) {
        abort();
    }

    memcpy(tmp, in1, len1);
    memcpy(tmp + len1, in2, len2);

    sha3_256(key, tmp, total);

    free(tmp);
}

/**
 * KDF_fail - SHA-3-256(z || ct)
 *
 * Используется для implicit rejection: если декапсуляция не прошла валидацию,
 * вместо m используется случайное значение z.
 */
void KDF_fail(uint8_t *key,
              const uint8_t *z,
              const uint8_t *ct,
              size_t ct_len)
{
    size_t total = Z_BYTES + ct_len;
    uint8_t *tmp = malloc(total);
    if (!tmp) {
        abort();
    }

    memcpy(tmp, z, Z_BYTES);
    memcpy(tmp + Z_BYTES, ct, ct_len);

    sha3_256(key, tmp, total);

    free(tmp);
}

/* ========================================================================
 * XOF (EXTENDABLE OUTPUT FUNCTION)
 * ======================================================================== */

/**
 * XOF - SHAKE128(seed) → out[outlen]
 *
 * Генерирует произвольное количество псевдослучайных байт из seed.
 */
void XOF(uint8_t *out, size_t outlen,
         const uint8_t *seed, size_t seedlen)
{
    shake128(out, outlen, seed, seedlen);
}

/* ========================================================================
 * ГЕНЕРАЦИЯ МАТРИЦЫ A
 * ======================================================================== */

/**
 * gen_matrix_A - обертка над GenMatrix из Reference_Implementation
 *
 * Генерирует публичную матрицу A размером SABER_L × SABER_L × SABER_N из seed.
 * Используется SHAKE128 для генерации коэффициентов.
 *
 * Оптимизация (только для FAST конфигурации):
 * - Если доступен SHAKE×3 NEON, используется параллельная генерация 3 строк
 * - Иначе используется последовательная генерация через GenMatrix()
 */

/* External function for Keccak×3 optimization */
/* Commented out - not currently used, GenMatrix is used instead */
/*
#ifdef __ARM_NEON
#include "../hash/keccak_times3_neon.h"
#endif
*/

void gen_matrix_A(uint16_t A[SABER_L][SABER_L][SABER_N],
                  const uint8_t seed[SABER_SEEDBYTES])
{
    /* Для совместимости с reference версией ВСЕГДА используем GenMatrix
     *
     * ПРОБЛЕМА: shake128_times3 требует РАЗНЫЕ входы для 3 параллельных SHAKE,
     * но GenMatrix генерирует ВСЮ матрицу из ОДНОГО seed через ОДИН shake128.
     *
     * Изменение алгоритма (разные seeds для каждого ряда) НАРУШИТ совместимость
     * с reference реализацией и тестами.
     *
     * TODO: Можно оптимизировать через:
     * 1. NEON-ускоренный single shake128 (feat.S уже есть)
     * 2. Параллелить генерацию СТОЛБЦОВ внутри ряда (9 полиномов → 3 группы по 3)
     */
    GenMatrix(A, seed);
}
