# SABER_GOST Architecture Documentation

## Модульная архитектура / Modular Architecture

SABER_GOST построен на принципе модульности с четкими границами между компонентами.

---

## Слои архитектуры / Architecture Layers

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                    │
│                 (User calls crypto_kem_*)               │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│                   KEM Layer (kem.c)                     │
│            Fujisaki-Okamoto Transform (FO)              │
│     • Saber_KeyGen()  • Saber_Encaps()                  │
│     • Saber_Decaps()                                    │
└─────┬─────────────┬──────────────┬────────────┬─────────┘
      │             │              │            │
   ┌──▼───┐   ┌────▼────┐   ┌─────▼────┐  ┌───▼────┐
   │ Core │   │  Hash   │   │   RNG    │  │ Verify │
   │ CPA  │   │ Module  │   │  Module  │  │ Module │
   └──┬───┘   └────┬────┘   └─────┬────┘  └────────┘
      │             │              │
   ┌──▼─────────────▼──────────────▼────────────────┐
   │         Polynomial Operations                   │
   │  • poly_mul()  • poly_add()  • poly_sub()       │
   │  • NTT/iNTT (FAST configs)                      │
   └─────────────────────────────────────────────────┘
```

---

## Компоненты по конфигурациям / Components by Configuration

### DEFAULT Configuration

```
kem.c (FO-transform)
  ├── core.c (CPA operations)
  │   ├── poly_toom.c (Toom-Cook multiplication)
  │   └── cbd, pack_unpack (reference)
  ├── hash_sha3.c (FIPS 202)
  │   └── fips202.c (Keccak)
  └── rng_system.c (system RNG)
```

### FAST_V4 Configuration (ARM64)

```
kem.c (FO-transform)
  ├── kem_fast_v4_optimized.c (prefetching)
  ├── core_neon_ntt_wrapper.c (API wrapper)
  │   └── neon-ntt/SABER_indcpa.c (asymmetric mul)
  │       ├── __asm_NTT.S, __asm_iNTT.S
  │       ├── __asm_mul.S (asymmetric multiplication)
  │       └── __asm_pack_unpack.S
  ├── poly_fast_v4_addon.c (NEON add/sub)
  ├── hash_sha3.c + fips202x2.c (parallel SHAKE)
  └── rng_system.c
```

### GOST Configuration

```
kem.c (FO-transform)
  ├── core.c (CPA operations)
  │   ├── poly_toom.c (Toom-Cook multiplication)
  │   └── cbd, pack_unpack (reference)
  ├── hash_gost.c (Streebog-512)
  └── rng_gost_ctr.c (Kuznyechik CTR-DRBG)
```

### GOST_FAST Configuration (ARM64)

```
kem.c (FO-transform)
  ├── core.c (CPA operations)
  │   ├── poly_ntt_neon.c (Incomplete-NTT, 747 lines)
  │   └── cbd, pack_unpack (reference)
  ├── hash_gost.c (Streebog-512)
  └── rng_gost_ctr.c (Kuznyechik CTR-DRBG)
```

---

## Модульные интерфейсы / Module Interfaces

### 1. Core CPA Interface (core.h)

```c
// CPA операции / CPA operations
void SaberCore_KeyGen(uint8_t *pk, uint8_t *sk);
void SaberCore_Encrypt(const uint8_t *pk, const uint8_t *m,
                       const uint8_t *coins, uint8_t *ct);
void SaberCore_Decrypt(const uint8_t *sk, const uint8_t *ct, uint8_t *m);
```

**Реализации**:
- `src/core/core.c` - DEFAULT/GOST (Toom-Cook)
- `src/kem/core_neon_ntt_wrapper.c` - FAST_V4 (neon-ntt wrapper)

### 2. Hash Interface (hash.h)

```c
// Хеш-функции / Hash functions
void HashSHA3_256(const uint8_t *in, size_t len, uint8_t *out);
void HashSHA3_512(const uint8_t *in, size_t len, uint8_t *out);
void SHAKE128(const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen);
```

**Реализации**:
- `src/hash/hash_sha3.c` - DEFAULT/FAST_V4 (FIPS 202)
- `src/hash/hash_gost.c` - GOST/GOST_FAST (Streebog-512)

### 3. RNG Interface (rng.h)

```c
// Генерация случайных байт / Random bytes generation
int randombytes(uint8_t *buf, size_t len);
```

**Реализации**:
- `src/rng/rng_system.c` - DEFAULT/FAST_V4 (system RNG)
- `src/rng/rng_gost_ctr.c` - GOST/GOST_FAST (Kuznyechik CTR-DRBG)

### 4. Polynomial Interface (poly.h)

```c
// Полиномиальные операции / Polynomial operations
void poly_mul(uint16_t c[SABER_N], const uint16_t a[SABER_N],
              const uint16_t b[SABER_N]);
void poly_add(uint16_t c[SABER_N], const uint16_t a[SABER_N],
              const uint16_t b[SABER_N]);
void poly_sub(uint16_t c[SABER_N], const uint16_t a[SABER_N],
              const uint16_t b[SABER_N]);

// Матричные операции / Matrix operations
void MatrixVectorMul(polyvec *out, const polyvec A[SABER_L],
                     const polyvec *s, int transpose);
void InnerProd(poly *out, const polyvec *a, const polyvec *b);
```

**Реализации**:
- `src/poly/poly_toom.c` - DEFAULT/GOST (Toom-Cook)
- `src/poly/poly_ntt_neon.c` - GOST_FAST (Incomplete-NTT)
- `neon-ntt/SABER_indcpa.c` - FAST_V4 (asymmetric mul)
- `src/poly/poly_fast_v4_addon.c` - FAST_V4 (NEON add/sub)

---

## Детали реализации / Implementation Details

### Fujisaki-Okamoto Transform (kem.c)

FO-transform преобразует CPA-схему в CCA-безопасный KEM:

```c
int Saber_KeyGen(uint8_t *pk, uint8_t *sk) {
    // 1. Генерация CPA ключей
    SaberCore_KeyGen(pk, sk);

    // 2. Хеш публичного ключа
    HashSHA3_256(pk, SABER_PUBLICKEYBYTES, sk + SABER_SECRETKEYBYTES);

    // 3. Случайное зерно для FO
    randombytes(sk + SABER_SECRETKEYBYTES + 32, SABER_KEYBYTES);

    return 0;
}

int Saber_Encaps(const uint8_t *pk, uint8_t *ct, uint8_t *ss) {
    // 1. Генерация случайного сообщения m
    randombytes(m, SABER_KEYBYTES);

    // 2. Хеш (m || pk) → coins
    HashSHA3_512(buf, SABER_KEYBYTES + SABER_PUBLICKEYBYTES, coins);

    // 3. CPA шифрование: ct = Encrypt(pk, m; coins)
    SaberCore_Encrypt(pk, m, coins, ct);

    // 4. Shared secret = Hash(m || ct)
    HashSHA3_256(buf, SABER_KEYBYTES + SABER_CIPHERTEXTBYTES, ss);

    return 0;
}

int Saber_Decaps(const uint8_t *sk, const uint8_t *ct, uint8_t *ss) {
    // 1. CPA дешифрование: m' = Decrypt(sk, ct)
    SaberCore_Decrypt(sk, ct, m);

    // 2. Повторное шифрование для проверки: ct' = Encrypt(pk, m'; coins')
    SaberCore_Encrypt(pk, m, coins, ct_re);

    // 3. Проверка ct == ct' (constant-time)
    int fail = verify(ct, ct_re, SABER_CIPHERTEXTBYTES);

    // 4. If fail: ss = Hash(z || ct), иначе ss = Hash(m || ct)
    cmov(m, sk + SABER_SECRETKEYBYTES + 32, SABER_KEYBYTES, fail);
    HashSHA3_256(buf, ss);

    return 0;
}
```

### Polynomial Multiplication Algorithms

#### 1. Toom-Cook (DEFAULT/GOST)

Классическое умножение полиномов O(n^1.465):

```c
void poly_mul_toomcook(uint16_t c[256], const uint16_t a[256],
                       const uint16_t b[256]) {
    // Разделение на 4 части: a = a0 + a1*x^64 + a2*x^128 + a3*x^192
    // Вычисление 7 точек: P0, P1, P_inf, P(-1), P(-2), P(1/2), P(2)
    // Интерполяция для получения результата
}
```

**Сложность**: ~3,500 умножений для 256 коэффициентов

#### 2. Incomplete-NTT (GOST_FAST)

NTT с 6 слоями для q=8192 (poly_ntt_neon.c):

```c
void poly_mul_ntt(uint16_t c[256], const uint16_t a[256],
                  const uint16_t b[256]) {
    // 1. Lift: Z_8192 → Z_25166081 (NTT-friendly prime)
    int32_t a_lifted[256], b_lifted[256];
    for (i = 0; i < 256; i++) {
        a_lifted[i] = (int32_t)a[i];
        b_lifted[i] = (int32_t)b[i];
    }

    // 2. NTT (6 layers): 256 → 64 blocks of 4
    ntt_incomplete(a_lifted);  // Vectorized with NEON
    ntt_incomplete(b_lifted);

    // 3. Pointwise multiplication (64 blocks)
    for (i = 0; i < 64; i++) {
        schoolbook_4x4(&result[i*4], &a_lifted[i*4], &b_lifted[i*4]);
    }

    // 4. Inverse NTT (6 layers)
    intt_incomplete(result);

    // 5. Reduce: Z_25166081 → Z_8192
    for (i = 0; i < 256; i++) {
        c[i] = (uint16_t)(result[i] & 0x1FFF);  // mod 8192
    }
}
```

**Сложность**: ~1,200 умножений (2.9× быстрее)

**NEON векторизация**:
```c
// Butterfly операция (4 коэффициента одновременно)
int32x4_t ntt_butterfly_neon(int32x4_t a, int32x4_t b, int32_t omega) {
    int32x4_t omega_vec = vdupq_n_s32(omega);
    int32x4_t t = vmulq_s32(b, omega_vec);
    t = montgomery_reduce_neon(t);
    return vsubq_s32(a, t);
}
```

#### 3. Asymmetric Multiplication (FAST_V4)

neon-ntt заменяет 9 умножений на 1 операцию:

```c
// Традиционный подход: 9 poly_mul для 3×3 матрицы
for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
        poly_mul(&result[i], &A[i][j], &s[j]);

// neon-ntt подход: 1 asymmetric_mul
__asm_asymmetric_mul(result, A, s);  // Assembly implementation
```

**Преимущества**:
- Единственная NTT/iNTT трансформация
- Оптимизированное использование регистров
- Минимизация memory transfers

---

## Параметры SABER / SABER Parameters

```c
#define SABER_L 3           // Размерность модуля / Module dimension
#define SABER_N 256         // Размерность полинома / Polynomial degree
#define SABER_Q 8192        // Модуль (2^13) / Modulus
#define SABER_P 1024        // Модуль сообщения (2^10) / Message modulus

#define SABER_EQ 13         // log2(q)
#define SABER_EP 10         // log2(p)
#define SABER_ET 3          // Rounding bits

#define SABER_MU 10         // Биты ошибки / Error bits

// Размеры ключей / Key sizes
#define SABER_PUBLICKEYBYTES  (SABER_L * SABER_N * 13 / 8)    // 992 bytes
#define SABER_SECRETKEYBYTES  (SABER_L * SABER_N * 13 / 8)    // 992 bytes
#define SABER_CIPHERTEXTBYTES (SABER_L * SABER_N * 10 / 8 + 64) // 1088 bytes
#define SABER_KEYBYTES        32                               // 256 bits
```

---

## Зависимости / Dependencies

### Внутренние (наши) / Internal (Ours)

- `src/kem.c` - FO-transform
- `src/core/core.c` - CPA операции
- `src/poly/poly_ntt_neon.c` - Incomplete-NTT (747 lines)
- `src/poly/poly_fast_v4_addon.c` - NEON poly ops (144 lines)
- `src/kem/kem_fast_v4_optimized.c` - ARM prefetching (226 lines)
- `src/hash/hash_gost.c` - ГОСТ Streebog
- `src/rng/rng_gost_ctr.c` - ГОСТ Kuznyechik CTR-DRBG

### Внешние / External

- `neon-ntt/` - neon-ntt implementation (Becker et al., TCHES 2022)
  - `SABER_indcpa.c` (242 lines)
  - `__asm_*.S` assembly files (1,726 lines)
  - `fips202x2.c` - Parallel SHAKE128 (588 lines)
- `SABER/Reference_Implementation_KEM/` - Original SABER reference
  - `poly_toom.c` - Toom-Cook multiplication
  - `verify.c` - Constant-time comparison
- `pqax/` - ARM Keccak assembly (450 lines)

---

## Процесс сборки / Build Process

### CMake конфигурация

```cmake
if(SABER_CONFIG STREQUAL "DEFAULT")
    # Использует poly_toom.c, hash_sha3.c, rng_system.c

elseif(SABER_CONFIG STREQUAL "FAST_V4" AND IS_ARM64)
    # Использует neon-ntt файлы + наши оптимизации
    # Требуется ARM64 для NEON инструкций

elseif(SABER_CONFIG STREQUAL "GOST")
    # Использует poly_toom.c, hash_gost.c, rng_gost_ctr.c

elseif(SABER_CONFIG STREQUAL "GOST_FAST" AND IS_ARM64)
    # Использует poly_ntt_neon.c, hash_gost.c, rng_gost_ctr.c
    # Требуется ARM64 для NEON инструкций
endif()
```

### Флаги компиляции

```bash
# Общие флаги / Common flags
-O3 -march=native -mtune=native

# ARM-specific
-march=armv8-a+simd+crypto

# NEON
-mfpu=neon (ARMv7)
```

---

## Безопасность / Security

### 1. Constant-time операции

```c
// Constant-time сравнение (verify.c)
int verify(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t r = 0;
    for (size_t i = 0; i < len; i++)
        r |= a[i] ^ b[i];
    return (-(uint64_t)r) >> 63;  // 0 if equal, 1 if different
}

// Constant-time conditional move (FO-transform)
void cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b) {
    b = -b;  // 0x00 or 0xFF
    for (size_t i = 0; i < len; i++)
        r[i] ^= b & (x[i] ^ r[i]);
}
```

### 2. Защита от timing attacks

- Все критические операции выполняются за константное время
- Отсутствие ветвлений зависящих от секретных данных
- NEON векторизация без data-dependent branches

### 3. Memory safety

- Отсутствие dynamic allocation в критическом пути
- Stack-based операции с фиксированными размерами
- Явная очистка чувствительных данных после использования

---

## Производительность / Performance Characteristics

### Hotspots (профилирование DEFAULT)

1. **poly_mul (Toom-Cook)**: ~60% времени
2. **MatrixVectorMul**: ~25% времени
3. **Hash operations**: ~10% времени
4. **Pack/unpack**: ~5% времени

### Оптимизации GOST_FAST

- poly_mul: 60% → 30% (Incomplete-NTT)
- MatrixVectorMul: 25% → 15% (NEON векторизация)
- **Общее ускорение**: 1.7-2×

### Оптимизации FAST_V4

- poly_mul: 60% → 20% (asymmetric mul)
- MatrixVectorMul: 25% → 10% (neon-ntt)
- Hash: 10% → 8% (fips202x2 parallel)
- **Общее ускорение**: 1.9-2×

---

## Тестирование / Testing

### Корректность / Correctness Tests

1. Базовые операции KEM (KeyGen, Encaps, Decaps)
2. Стабильность (100 итераций)
3. Уникальность (различные входы → различные выходы)
4. Детерминированность (одинаковые входы → одинаковые выходы)
5. Целостность (модифицированный CT → различные SS)
6. Граничные случаи

### Производительность / Performance Benchmarks

- Микробенчмарки отдельных операций
- Полные KeyGen/Encaps/Decaps циклы
- Сравнение между конфигурациями

---

## Расширения / Extensions

Архитектура поддерживает добавление:

1. Новых хеш-функций (реализация hash.h интерфейса)
2. Новых RNG (реализация rng.h интерфейса)
3. Новых полиномиальных умножений (реализация poly.h)
4. Дополнительных оптимизаций (AVX2, AVX-512, SVE)

---

**Архитектура спроектирована для максимальной модульности, производительности и безопасности** 🏗️
