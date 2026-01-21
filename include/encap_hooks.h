#ifndef SABER_ENCAP_HOOKS_H
#define SABER_ENCAP_HOOKS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "params.h"
#include <stdlib.h>
#include "../external/saber_ref/SABER_params.h"
#include "../external/saber_ref/rng.h"
#include "../external/saber_ref/fips202.h"
#include "../external/saber_ref/SABER_indcpa.h"
#include "../external/saber_ref/pack_unpack.h"

/*
   Все низкоуровневые операции (случайные байты, хеши, KDF‐fail,
   генерация «coins» в FO‐стиле, проверка, re‐encrypt и т. д.)
   вынесены сюда.
*/

//-----------------------------------------------------------------------------
// Внутренние типы «core»
typedef struct {
    uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];  // CPA‐секретный ключ
    uint8_t z [Z_BYTES];                      // 32‐байтный секрет для KDF_fail
} sk_core_t;

typedef struct {
    uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];  // CPA‐публичный ключ
} pk_core_t;

typedef struct {
    uint8_t ct[SABER_BYTES_CCA_DEC];          // CPA‐шифротекст
} ct_core_t;


//-----------------------------------------------------------------------------
// Основные «core» операции (реализованы в encap_hooks.c)
void SaberCore_KeyGen(pk_core_t *pk_core, sk_core_t *sk_core);

// Core_Encrypt: шифруем m, используя заранее подготовленные coins
void SaberCore_Encrypt(ct_core_t    *c_core,
                       const pk_core_t *pk_core,
                       const uint8_t   *m,
                       const uint8_t   *coins);

// Core_Decrypt: просто CPA‐расшифровка
void SaberCore_Decrypt(uint8_t        *m,
                       const sk_core_t *sk_core,
                       const ct_core_t *c_core);


//-----------------------------------------------------------------------------
// Сериализация / десериализация (тот же вариант, что и раньше)
static inline void serialize_pk(uint8_t *pk, const pk_core_t *pk_core) {
    memcpy(pk, pk_core->pk, SABER_INDCPA_PUBLICKEYBYTES);
}

static inline void deserialize_pk(pk_core_t *pk_core, const uint8_t *pk) {
    memcpy(pk_core->pk, pk, SABER_INDCPA_PUBLICKEYBYTES);
}

static inline void serialize_ct(uint8_t *ct,
                                const ct_core_t *c_core,
                                const uint8_t   *d) {
    memcpy(ct, c_core->ct, SABER_BYTES_CCA_DEC);
    memcpy(ct + SABER_BYTES_CCA_DEC, d, D_BYTES);
}

static inline void deserialize_ct(ct_core_t *c_core,
                                  uint8_t   *d,
                                  const uint8_t *ct) {
    memcpy(c_core->ct, ct, SABER_BYTES_CCA_DEC);
    memcpy(d, ct + SABER_BYTES_CCA_DEC, D_BYTES);
}


//-----------------------------------------------------------------------------
// Случайные байты
//   Вызываем underlying randombytes() из rng.h
static inline void random_bytes(uint8_t *buf, size_t len) {
    randombytes(buf, len);
}


//-----------------------------------------------------------------------------
// Хеш-функции H1 и H2
//   Здесь H1 = SHA3_256(m || c_core), H2 = SHA3_256(m || c_core).
//   Реализовано через sha3_256 из fips202.h
void H1(uint8_t *digest,
        const uint8_t *in1, size_t in1_len,
        const uint8_t *in2, size_t in2_len);

void H2(uint8_t *key,
        const uint8_t *in1, size_t in1_len,
        const uint8_t *in2, size_t in2_len);


//-----------------------------------------------------------------------------
// KDF при провале: key = SHA3_256(z || c_core)
void KDF_fail(uint8_t         *key,
              const sk_core_t *sk_core,
              const uint8_t   *c_core, size_t c_len);


//-----------------------------------------------------------------------------
// FO‐специфичные вспомогательные функции
//
// generate_coins(m, pk, coins):
//   Реализует «seed = SHA3(m)»; «coins = SHA3(seed || pk)».
//   Выдаёт массив length=NOISE_BYTES.
//   (msg_len = MSG_BYTES, pk_len = SABER_INDCPA_PUBLICKEYBYTES)
//
// compute_d(m, c_core, d):
//   Простейший вызов H1(m, c_core) → d (D_BYTES).
//
// compute_shared(m, c_core, shared_key):
//   Простейший вызов H2(m, c_core) → shared_key (SABER_SHARED_KEY_BYTES).
//
// Все эти функции используются из kem.c вместо прямого вызова sha3_256.
void generate_coins(uint8_t       *coins,
                    const uint8_t *m,
                    const uint8_t *pk);

void compute_d(const uint8_t   *m,
               const ct_core_t *c_core,
               uint8_t         *d);

void compute_shared(const uint8_t   *m,
                    const ct_core_t *c_core,
                    uint8_t         *shared_key);


#endif // SABER_ENCAP_HOOKS_H
