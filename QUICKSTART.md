# SABER_GOST Quick Start Guide

## Быстрый старт за 3 минуты / 3-Minute Quick Start

### 1️⃣ Сборка / Build

```bash
cd scripts
./build_all.sh
```

Это создаст 4 директории сборки:
- `build_default` - DEFAULT конфигурация
- `build_fast_v4` - FAST_V4 конфигурация (ARM64)
- `build_gost` - GOST конфигурация
- `build_gost_fast` - GOST_FAST конфигурация (ARM64)

### 2️⃣ Тестирование / Test

```bash
./test_all.sh
```

Ожидаемый результат: **10/10 тестов для каждой конфигурации** ✅

### 3️⃣ Бенчмарк / Benchmark

```bash
./bench_all.sh
```

---

## Использование API / API Usage

### Базовое использование / Basic Usage

```c
#include "api.h"

int main() {
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_alice[CRYPTO_BYTES];
    uint8_t ss_bob[CRYPTO_BYTES];

    // 1. Генерация ключей (Bob)
    crypto_kem_keypair(pk, sk);

    // 2. Инкапсуляция (Alice)
    crypto_kem_enc(ct, ss_alice, pk);

    // 3. Декапсуляция (Bob)
    crypto_kem_dec(ss_bob, ct, sk);

    // 4. Проверка
    if (memcmp(ss_alice, ss_bob, CRYPTO_BYTES) == 0) {
        printf("Success! Shared secrets match.\n");
    }

    return 0;
}
```

### Компиляция приложения / Compiling Your Application

```bash
# DEFAULT конфигурация
cd build_default
cmake -DSABER_CONFIG=DEFAULT ..
make
gcc your_app.c -I../include -Lbuild_default -lsaber_gost -o your_app

# FAST_V4 (ARM64)
cd build_fast_v4
cmake -DSABER_CONFIG=FAST_V4 ..
make
gcc your_app.c -I../include -Lbuild_fast_v4 -lsaber_gost -o your_app

# GOST конфигурация
cd build_gost
cmake -DSABER_CONFIG=GOST ..
make
gcc your_app.c -I../include -Lbuild_gost -lsaber_gost -o your_app

# GOST_FAST (ARM64)
cd build_gost_fast
cmake -DSABER_CONFIG=GOST_FAST ..
make
gcc your_app.c -I../include -Lbuild_gost_fast -lsaber_gost -o your_app
```

---

## Размеры ключей / Key Sizes

| Параметр | Размер |
|----------|--------|
| Public Key | 992 bytes |
| Secret Key | 2304 bytes |
| Ciphertext | 1088 bytes |
| Shared Secret | 32 bytes (256 bits) |

---

## Выбор конфигурации / Configuration Selection

| Конфигурация | Когда использовать |
|--------------|-------------------|
| **DEFAULT** | Максимальная совместимость, любые платформы |
| **FAST_V4** | ARM64 сервера/устройства, нужна максимальная производительность (2× быстрее) |
| **GOST** | Требуется совместимость с ГОСТ стандартами, любые платформы |
| **GOST_FAST** | ARM64 + ГОСТ стандарты (1.7-2× быстрее GOST) |

---

## Примеры производительности / Performance Examples

### Apple M4 Max (ARMv9.2-A)

```
DEFAULT:     15 μs (KeyGen) | 19 μs (Encaps) | 23 μs (Decaps)
FAST_V4:      8 μs (KeyGen) | 10 μs (Encaps) | 11 μs (Decaps)  ⚡ 1.84-2× faster
GOST:        18 μs (KeyGen) | 26 μs (Encaps) | 29 μs (Decaps)
GOST_FAST:   18 μs (KeyGen) | 26 μs (Encaps) | 29 μs (Decaps)  (no speedup on M4)
```

### Типичный ARM сервер (AWS Graviton, Ampere Altra)

```
FAST_V4:     ~1.84-2× быстрее DEFAULT
GOST_FAST:   ~1.5-1.8× быстрее GOST (на серверных ARM)
```

---

## Структура проекта / Project Structure

```
SABER_GOST_PRODUCTION/
├── README.md              ← Главная документация
├── QUICKSTART.md          ← Этот файл
├── CMakeLists.txt         ← Сборка проекта
├── src/                   ← Исходный код
├── include/               ← Заголовочные файлы
├── tests/                 ← Тесты и бенчмарки
├── scripts/               ← Скрипты сборки/тестирования
└── docs/                  ← Детальная документация
    ├── ARCHITECTURE.md    ← Архитектура
    ├── CODE_AUTHORSHIP_AUDIT.md
    ├── CONFIGURATION_COMPARISON.md
    └── FAST_V4_PERFORMANCE_RESULTS.md
```

---

## Требования / Requirements

### Минимальные требования

- **Компилятор**: GCC 9+ или Clang 12+
- **CMake**: 3.15+
- **Платформа**: Любая для DEFAULT/GOST

### Для FAST_V4/GOST_FAST

- **Архитектура**: ARM64 (ARMv8-A+)
- **NEON**: Поддержка SIMD инструкций
- **Рекомендуется**: ARMv8.2+ для лучшей производительности

---

## Устранение проблем / Troubleshooting

### Ошибка: "neon-ntt files not found"

FAST_V4 требует внешние файлы neon-ntt. Убедитесь, что вы клонировали полный репозиторий:

```bash
git clone --recursive https://github.com/yourusername/SABER_GOST
```

### Ошибка: "NEON instructions not supported"

FAST_V4/GOST_FAST требуют ARM64 архитектуру. Используйте DEFAULT или GOST для других платформ.

### Тесты проваливаются

```bash
# Пересборка с чистого листа
rm -rf build_*
cd scripts
./build_all.sh
./test_all.sh
```

---

## Следующие шаги / Next Steps

1. **Прочитайте README.md** для полного понимания проекта
2. **Изучите ARCHITECTURE.md** для детального понимания архитектуры
3. **Запустите бенчмарки** для оценки производительности на вашей платформе
4. **Интегрируйте в свое приложение** используя примеры выше

---

## Поддержка / Support

- **Issues**: https://github.com/yourusername/SABER_GOST/issues
- **Email**: krotovnikolay@example.com

**Готовы к постквантовой безопасности!** 🔐🚀
