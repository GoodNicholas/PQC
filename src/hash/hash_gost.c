/**
 * @file hash_gost.c
 * @brief GOST R 34.11-2012 (Streebog) implementation of hash module for SABER_GOST
 *
 * Uses Streebog-256 from engine/libgost.
 * This is the hash implementation for GOST and GOST_FAST configurations.
 *
 * Implements:
 * - H1/H2/KDF_fail through Streebog-256
 * - XOF through Streebog-256 counter mode
 * - gen_matrix_A through Streebog-XOF
 */

#include "../../include/hash.h"
#include "../../include/config.h"
#include "../../include/params.h"
#include "../../../engine/gosthash2012.h"
#include "../../external/saber_ref/poly.h"
#include "../../external/saber_ref/pack_unpack.h"
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ: STREEBOG-256
 * ======================================================================== */

/**
 * streebog256 - обертка над Streebog-256 из libgost
 *
 * Вычисляет hash = Streebog-256(data).
 */
static void streebog256(uint8_t *hash, const uint8_t *data, size_t len)
{
    gost2012_hash_ctx ctx;

    /* Инициализация для 256-битного хэша */
    init_gost2012_hash_ctx(&ctx, 256);

    /* Обработка данных */
    gost2012_hash_block(&ctx, data, len);

    /* Финализация и получение результата */
    gost2012_finish_hash(&ctx, hash);
}

/* ========================================================================
 * ОСНОВНЫЕ ХЭШ-ФУНКЦИИ
 * ======================================================================== */

/**
 * H1 - Streebog-256(in1 || in2)
 */
void H1(uint8_t *digest,
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

    streebog256(digest, tmp, total);

    free(tmp);
}

/**
 * H2 - Streebog-256(in1 || in2)
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

    streebog256(key, tmp, total);

    free(tmp);
}

/**
 * KDF_fail - Streebog-256(z || ct)
 *
 * Используется для implicit rejection.
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

    streebog256(key, tmp, total);

    free(tmp);
}

/* ========================================================================
 * XOF (EXTENDABLE OUTPUT FUNCTION) - STREEBOG COUNTER MODE
 * ======================================================================== */

/**
 * XOF - Streebog-256 в режиме счетчика (counter mode)
 *
 * Алгоритм (из плана):
 * XOF(seed, n) = Streebog(seed || 0) || Streebog(seed || 1) || ... || Streebog(seed || k-1)
 * где k = ⌈n/32⌉ для Streebog-256
 * Счетчик: 4-байтовое целое в формате little-endian
 *
 * Генерирует произвольное количество псевдослучайных байт из seed.
 */
void XOF(uint8_t *out, size_t outlen,
         const uint8_t *seed, size_t seedlen)
{
    uint8_t block[32];  /* Streebog-256 дает 32 байта */
    size_t offset = 0;
    uint32_t counter = 0;

    /* Используем статический буфер на стеке вместо malloc */
    /* Максимальный размер seed = SABER_SEEDBYTES + 2 (для gen_matrix_A) */
    uint8_t tmp[SABER_SEEDBYTES + 2 + 4];  /* seed + matrix indices + counter */

    if (seedlen > sizeof(tmp) - 4) {
        abort();  /* Защита от переполнения */
    }

    /* Копируем seed в начало */
    memcpy(tmp, seed, seedlen);

    /* Генерируем блоки пока не заполним output */
    while (offset < outlen) {
        /* Добавляем counter в little-endian формате */
        tmp[seedlen + 0] = (counter >> 0) & 0xFF;
        tmp[seedlen + 1] = (counter >> 8) & 0xFF;
        tmp[seedlen + 2] = (counter >> 16) & 0xFF;
        tmp[seedlen + 3] = (counter >> 24) & 0xFF;

        /* Вычисляем блок: block = Streebog-256(seed || counter) */
        streebog256(block, tmp, seedlen + 4);

        /* Копируем нужное количество байт в output */
        size_t tocopy = (outlen - offset < 32) ? (outlen - offset) : 32;
        memcpy(out + offset, block, tocopy);

        offset += tocopy;
        counter++;
    }
    /* Больше не нужен free, так как tmp на стеке */
}

/* ========================================================================
 * ГЕНЕРАЦИЯ МАТРИЦЫ A
 * ======================================================================== */

/**
 * gen_matrix_A - генерация публичной матрицы A из seed через Streebog-XOF
 *
 * Для GOST конфигурации используется Streebog-XOF вместо SHAKE128.
 * Генерирует матрицу A размером SABER_L × SABER_L × SABER_N из seed.
 *
 * Алгоритм:
 * 1. Для каждого элемента матрицы A[i][j] генерируется уникальный seed'
 * 2. seed' = seed || i || j (добавляем индексы для уникальности)
 * 3. Используем XOF(seed') для генерации SABER_N коэффициентов
 * 4. Применяем модульное сжатие к q = 8192
 */
/* External SHAKE×3 function for fast matrix generation */
#ifdef __ARM_NEON
extern void gen_matrix_A_shake3(uint16_t A[SABER_L][SABER_L][SABER_N],
                                const uint8_t seed[SABER_SEEDBYTES]);
#endif

void gen_matrix_A(uint16_t A[SABER_L][SABER_L][SABER_N],
                  const uint8_t seed[SABER_SEEDBYTES])
{
#if 0 /* Disabled for now - needs different approach for Keccak×3 */
    /*
     * ГИБРИДНЫЙ ПОДХОД для GOST:
     * - Используем быстрый SHAKE×3 для генерации матрицы (публичная операция)
     * - Сохраняем Streebog для H1, H2, KDF (криптографические функции)
     *
     * Это не нарушает стандарт ГОСТ, так как матрица A - публичный параметр,
     * который может генерироваться любым детерминированным способом.
     */
    gen_matrix_A_shake3(A, seed);
#else
    /*
     * Fallback: генерируем строки последовательно через Streebog-XOF
     */
    for (int i = 0; i < SABER_L; i++) {
        /* Создаем уникальный seed для каждой строки матрицы */
        uint8_t extended_seed[SABER_SEEDBYTES + 1];
        memcpy(extended_seed, seed, SABER_SEEDBYTES);
        extended_seed[SABER_SEEDBYTES] = (uint8_t)i;

        /* Генерируем SABER_POLYVECBYTES байт для всей строки через Streebog-XOF */
        uint8_t row_bytes[SABER_POLYVECBYTES];
        XOF(row_bytes, SABER_POLYVECBYTES, extended_seed, SABER_SEEDBYTES + 1);

        /* Используем оптимизированную функцию распаковки из Reference Implementation */
        /* BS2POLVECq распаковывает SABER_POLYVECBYTES байт в SABER_L полиномов */
        BS2POLVECq(row_bytes, A[i]);
    }
#endif
}
