/**
 * @file kem.c
 * @brief CCA-secure KEM using FO-transformation for SABER_GOST
 *
 * Implements Fujisaki-Okamoto transformation to convert CPA-secure Saber
 * into a CCA-secure KEM (Key Encapsulation Mechanism).
 *
 * Matches the reference implementation's FO transformation.
 */

#include <string.h>
#include "../include/api.h"
#include "../include/core.h"
#include "../include/hash.h"
#include "../include/rng.h"
#include "../include/fo_utils.h"
#include "../include/params.h"

/* Размеры структур */
#define PK_BYTES         SABER_INDCPA_PUBLICKEYBYTES


//-----------------------------------------------------------------------------
// KeyGen: core + сериализация
//-----------------------------------------------------------------------------
/**
 * Saber_KeyGen - генерация пары ключей CCA-Saber KEM
 *
 * Структура ключей (matching reference implementation):
 * - pk: [seed_A || b] размером SABER_PUBLIC_KEY_BYTES (992 байта)
 * - sk: [s || pk || h(pk) || z] размером SABER_SECRET_KEY_BYTES (2304 байта)
 *   где s     - CPA секретный ключ (1248 байт)
 *       pk    - копия публичного ключа (992 байта)
 *       h(pk) - хэш публичного ключа (32 байта)
 *       z     - случайное значение для implicit rejection (32 байта)
 */
int Saber_KeyGen(uint8_t *pk, uint8_t *sk) {
    uint8_t z[Z_BYTES];

    /* 1. Генерация CPA ключей через indcpa_kem_keypair */
    /*    pk = [seed_A || b], sk[0:1247] = s */
    SaberCore_KeyGen(pk, sk);

    /* 2. Добавляем копию pk после s */
    /*    sk = [s || pk || ...] */
    memcpy(sk + SABER_INDCPA_SECRETKEYBYTES, pk, PK_BYTES);

    /* 3. Вычисляем h(pk) и добавляем после pk */
    /*    sk = [s || pk || h(pk) || ...] */
    H2(sk + SABER_INDCPA_SECRETKEYBYTES + PK_BYTES, pk, PK_BYTES, NULL, 0);

    /* 4. Генерируем z и добавляем в конец sk */
    /*    sk = [s || pk || h(pk) || z] */
    random_bytes(z, Z_BYTES);
    memcpy(sk + SABER_INDCPA_SECRETKEYBYTES + PK_BYTES + SABER_HASHBYTES, z, Z_BYTES);

    return 0;
}


//-----------------------------------------------------------------------------
// Encaps: FO‐преобразование + core‐шифрование
//-----------------------------------------------------------------------------
/**
 * Saber_Encaps - инкапсуляция ключа
 *
 * Алгоритм FO-трансформации (matching reference):
 * 1. m ← random(MSG_BYTES)
 * 2. coins = generate_coins(m, pk)
 * 3. ct = SaberCore_Encrypt(pk, m; coins)
 * 4. shared_key = H2(m || ct)
 */
int Saber_Encaps(const uint8_t *pk, uint8_t *ct, uint8_t *shared_key) {
    uint8_t m[MSG_BYTES];
    uint8_t coins[NOISE_BYTES];

    /* 1. Сгенерируем случайное сообщение m */
    random_bytes(m, MSG_BYTES);

    /* 2. FO-шаг: детерминированная генерация монет */
    generate_coins(coins, m, pk);

    /* 3. CPA-шифрование: ct = Enc(pk, m; coins) */
    SaberCore_Encrypt(pk, m, coins, ct);

    /* 4. Вычисляем общий ключ shared_key = H2(m || ct) */
    compute_shared(m, ct, shared_key);

    return 0;
}


//-----------------------------------------------------------------------------
// Decaps: FO‐преобразование + core‐расшифровка + re‐encrypt check
//-----------------------------------------------------------------------------
/**
 * Saber_Decaps - декапсуляция ключа
 *
 * Алгоритм FO-трансформации (matching reference):
 * 1. Распарсить sk = [s || pk || h(pk) || z]
 * 2. m' = SaberCore_Decrypt(s, ct)
 * 3. coins' = generate_coins(m', pk)
 * 4. ct' = SaberCore_Encrypt(pk, m'; coins')
 * 5. Если ct != ct': m' := z (implicit rejection)
 * 6. shared_key = H2(m' || ct)
 */
int Saber_Decaps(const uint8_t *sk, const uint8_t *ct, uint8_t *shared_key) {
    /* Распарсить sk = [s || pk || h(pk) || z] */
    const uint8_t *sk_s = sk;                                                /* s */
    const uint8_t *sk_pk = sk + SABER_INDCPA_SECRETKEYBYTES;                /* pk */
    const uint8_t *sk_z = sk + SABER_INDCPA_SECRETKEYBYTES + PK_BYTES + SABER_HASHBYTES;  /* z */

    uint8_t m_prime[MSG_BYTES];
    uint8_t coins_prime[NOISE_BYTES];
    uint8_t ct_prime[SABER_CIPHERTEXT_BYTES];

    /* 1. CPA-дешифрование: m' = Dec(s, ct) */
    SaberCore_Decrypt(sk_s, ct, m_prime);

    /* 2. FO-шаг: восстанавливаем coins' = generate_coins(m', pk) */
    generate_coins(coins_prime, m_prime, sk_pk);

    /* 3. Re-encrypt: ct' = Enc(pk, m'; coins') */
    SaberCore_Encrypt(sk_pk, m_prime, coins_prime, ct_prime);

    /* 4. Проверка корректности: ct == ct' */
    int fail = (memcmp(ct, ct_prime, SABER_CIPHERTEXT_BYTES) != 0);

    /* 5. Implicit rejection: если проверка не прошла, используем z вместо m' */
    if (fail) {
        memcpy(m_prime, sk_z, MSG_BYTES);
    }

    /* 6. Вычисляем общий ключ shared_key = H2(m' || ct) */
    compute_shared(m_prime, ct, shared_key);

    return 0;
}
