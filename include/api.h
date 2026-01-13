#ifndef SABER_API_H
#define SABER_API_H

#include <stdint.h>
#include "../../SABER/Reference_Implementation_KEM/SABER_params.h"
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

#ifdef __cplusplus
}
#endif

#endif //SABER_API_H
