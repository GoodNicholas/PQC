#ifndef SABER_API_H
#define SABER_API_H

#include <stdint.h>
#include "../external/saber_ref/SABER_params.h"
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Генерация пары ключей Saber KEM.
 * @param[out] pk   Буфер размера SABER_PUBLIC_KEY_BYTES для публичного ключа.
 * @param[out] sk   Буфер размера SABER_SECRET_KEY_BYTES для секретного ключа.
 * @return 0 в случае успеха.
 */
int Saber_KeyGen(uint8_t *pk, uint8_t *sk);

/**
 * @brief Инкапсуляция: по публичному ключу генерируется шифротекст и общий секрет.
 * @param[in]  pk         Публичный ключ (SABER_PUBLIC_KEY_BYTES).
 * @param[out] ct         Буфер размера SABER_CIPHERTEXT_BYTES для шифротекста.
 * @param[out] shared_key Буфер размера SABER_SHARED_KEY_BYTES для общего секрета.
 * @return 0 в случае успеха.
 */
int Saber_Encaps(const uint8_t *pk, uint8_t *ct, uint8_t *shared_key);

/**
 * @brief Декапсуляция: по секретному ключу и шифротексту восстанавливается общий секрет.
 * @param[in]  sk         Секретный ключ (SABER_SECRET_KEY_BYTES).
 * @param[in]  ct         Шифротекст (SABER_CIPHERTEXT_BYTES).
 * @param[out] shared_key Буфер размера SABER_SHARED_KEY_BYTES для общего секрета.
 * @return 0 в случае успеха.
 */
int Saber_Decaps(const uint8_t *sk, const uint8_t *ct, uint8_t *shared_key);

// ========================================================================
// BATCHING API (доступно при компиляции с -DSABER_BATCHING_ENABLED)
// ========================================================================

#ifdef SABER_BATCHING_ENABLED

/**
 * @brief Пакетная генерация двух пар ключей Saber KEM.
 *
 * При поддержке батчинга (NEON, AVX2) эта функция может обрабатывать
 * две операции генерации ключей более эффективно, чем два отдельных вызова.
 *
 * @param[out] pk0, pk1  Буферы для публичных ключей (SABER_PUBLICKEYBYTES каждый)
 * @param[out] sk0, sk1  Буферы для секретных ключей (SABER_SECRETKEYBYTES каждый)
 * @return 0 в случае успеха
 */
int saber_batch2_keygen(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES]
);

/**
 * @brief Пакетная инкапсуляция для двух публичных ключей.
 *
 * @param[out] ct0, ct1  Буферы для шифротекстов (SABER_BYTES_CCA_DEC каждый)
 * @param[out] ss0, ss1  Буферы для общих секретов (SABER_KEYBYTES каждый)
 * @param[in]  pk0, pk1  Публичные ключи (SABER_PUBLICKEYBYTES каждый)
 * @return 0 в случае успеха
 */
int saber_batch2_encaps(
    uint8_t ct0[SABER_BYTES_CCA_DEC],
    uint8_t ct1[SABER_BYTES_CCA_DEC],
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t pk0[SABER_PUBLICKEYBYTES],
    const uint8_t pk1[SABER_PUBLICKEYBYTES]
);

/**
 * @brief Пакетная декапсуляция для двух шифротекстов.
 *
 * @param[out] ss0, ss1  Буферы для общих секретов (SABER_KEYBYTES каждый)
 * @param[in]  ct0, ct1  Шифротексты (SABER_BYTES_CCA_DEC каждый)
 * @param[in]  sk0, sk1  Секретные ключи (SABER_SECRETKEYBYTES каждый)
 * @return 0 в случае успеха
 */
int saber_batch2_decaps(
    uint8_t ss0[SABER_KEYBYTES],
    uint8_t ss1[SABER_KEYBYTES],
    const uint8_t ct0[SABER_BYTES_CCA_DEC],
    const uint8_t ct1[SABER_BYTES_CCA_DEC],
    const uint8_t sk0[SABER_SECRETKEYBYTES],
    const uint8_t sk1[SABER_SECRETKEYBYTES]
);

/**
 * @brief Пакетная генерация четырех пар ключей Saber KEM (SaberX4).
 *
 * Использует 2x shake128x2 для батчирования операций хеширования.
 * Дает ~1.1x throughput speedup на ARM при batch size 4.
 *
 * @param[out] pk0-3  Буферы для публичных ключей (SABER_PUBLICKEYBYTES каждый)
 * @param[out] sk0-3  Буферы для секретных ключей (SABER_SECRETKEYBYTES каждый)
 * @return 0 в случае успеха
 */
int saber_batch4_keygen(
    uint8_t pk0[SABER_PUBLICKEYBYTES],
    uint8_t pk1[SABER_PUBLICKEYBYTES],
    uint8_t pk2[SABER_PUBLICKEYBYTES],
    uint8_t pk3[SABER_PUBLICKEYBYTES],
    uint8_t sk0[SABER_SECRETKEYBYTES],
    uint8_t sk1[SABER_SECRETKEYBYTES],
    uint8_t sk2[SABER_SECRETKEYBYTES],
    uint8_t sk3[SABER_SECRETKEYBYTES]
);

#endif // SABER_BATCHING_ENABLED

#ifdef __cplusplus
}
#endif

#endif //SABER_API_H
