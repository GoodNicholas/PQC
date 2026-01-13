#ifndef SABER_CONFIG_H
#define SABER_CONFIG_H

/**
 * @file config.h
 * @brief Система конфигураций для SABER_GOST
 *
 * Поддерживаемые конфигурации (выбираются через CMake):
 * - DEFAULT:    SHA-3, System RNG, Toom-Cook (базовая реализация)
 * - FAST:       SHAKE×4 NEON, ChaCha20 NEON, NTT-Incomplete (оптимизация для ARM64)
 * - GOST:       Streebog, Kuznyechik CTR, Toom-Cook, Batch coins (ГОСТ-криптография)
 * - GOST_FAST:  Streebog, Kuznyechik CTR, NTT-Incomplete, Batch coins (ГОСТ + ARM64)
 * - TEST:       SHA-3, CTR_DRBG, Toom-Cook (детерминированное тестирование)
 */

/* ========================================================================
 * ПРОВЕРКА КОНФИГУРАЦИИ
 * ======================================================================== */

#if defined(SABER_CONFIG_DEFAULT) + defined(SABER_CONFIG_FAST) + \
    defined(SABER_CONFIG_GOST) + defined(SABER_CONFIG_GOST_FAST) + \
    defined(SABER_CONFIG_TEST) > 1
    #error "Можно определить только одну конфигурацию SABER_CONFIG_*"
#endif

#if !defined(SABER_CONFIG_DEFAULT) && !defined(SABER_CONFIG_FAST) && \
    !defined(SABER_CONFIG_GOST) && !defined(SABER_CONFIG_GOST_FAST) && \
    !defined(SABER_CONFIG_TEST)
    #define SABER_CONFIG_DEFAULT  /* По умолчанию используется DEFAULT */
    #warning "Конфигурация не указана, используется SABER_CONFIG_DEFAULT"
#endif

/* ========================================================================
 * КОНФИГУРАЦИЯ: DEFAULT
 * ======================================================================== */
#ifdef SABER_CONFIG_DEFAULT
    /* Hash module: SHA-3 */
    #define USE_HASH_SHA3

    /* RNG module: System RNG */
    #define USE_RNG_SYSTEM

    /* Polynomial module: Toom-Cook */
    #define USE_POLY_TOOM

    /* FO utils module: Reference implementation */
    #define USE_FO_UTILS_REF
#endif

/* ========================================================================
 * КОНФИГУРАЦИЯ: FAST
 * ======================================================================== */
#ifdef SABER_CONFIG_FAST
    /* Hash module: SHA-3 + SHAKE×4 NEON для gen_matrix_A */
    #define USE_HASH_SHA3
    #define USE_SHAKE4X_NEON

    /* RNG module: ChaCha20 NEON */
    #define USE_RNG_CHACHA_NEON

    /* Polynomial module: NTT-Incomplete NEON */
    #define USE_POLY_NTT_NEON

    /* FO utils module: ChaCha20 NEON для generate_coins */
    #define USE_FO_UTILS_CHACHA_NEON
#endif

/* ========================================================================
 * КОНФИГУРАЦИЯ: GOST
 * ======================================================================== */
#ifdef SABER_CONFIG_GOST
    /* Hash module: Streebog (ГОСТ Р 34.11-2012) */
    #define USE_HASH_GOST

    /* RNG module: Kuznyechik CTR (ГОСТ Р 34.12-2015) */
    #define USE_RNG_GOST_CTR

    /* Polynomial module: Toom-Cook */
    #define USE_POLY_TOOM

    /* FO utils module: Batch generation */
    #define USE_FO_UTILS_BATCH
#endif

/* ========================================================================
 * КОНФИГУРАЦИЯ: GOST_FAST
 * ======================================================================== */
#ifdef SABER_CONFIG_GOST_FAST
    /* Hash module: Streebog (ГОСТ Р 34.11-2012) */
    #define USE_HASH_GOST

    /* RNG module: Kuznyechik CTR (ГОСТ Р 34.12-2015) */
    #define USE_RNG_GOST_CTR

    /* Polynomial module: NTT-Incomplete NEON */
    #define USE_POLY_NTT_NEON

    /* FO utils module: Batch generation */
    #define USE_FO_UTILS_BATCH
#endif

/* ========================================================================
 * КОНФИГУРАЦИЯ: TEST
 * ======================================================================== */
#ifdef SABER_CONFIG_TEST
    /* Hash module: SHA-3 */
    #define USE_HASH_SHA3

    /* RNG module: CTR_DRBG (детерминированный) */
    #define USE_RNG_CTR_DRBG

    /* Polynomial module: Toom-Cook */
    #define USE_POLY_TOOM

    /* FO utils module: Reference implementation */
    #define USE_FO_UTILS_REF
#endif

/* ========================================================================
 * ПРОВЕРКА ПОДДЕРЖКИ NEON
 * ======================================================================== */
#if defined(USE_SHAKE4X_NEON) || defined(USE_RNG_CHACHA_NEON) || \
    defined(USE_POLY_NTT_NEON) || defined(USE_FO_UTILS_CHACHA_NEON)
    #ifndef __ARM_NEON
        #warning "NEON оптимизации запрошены, но __ARM_NEON не определен. Используется fallback."

        /* Fallback на reference implementations */
        #undef USE_SHAKE4X_NEON
        #undef USE_RNG_CHACHA_NEON
        #undef USE_POLY_NTT_NEON
        #undef USE_FO_UTILS_CHACHA_NEON

        /* Переключаемся на базовые реализации */
        #ifdef SABER_CONFIG_FAST
            #define USE_RNG_SYSTEM
            #define USE_POLY_TOOM
            #define USE_FO_UTILS_REF
        #endif

        #ifdef SABER_CONFIG_GOST_FAST
            #define USE_POLY_TOOM
        #endif
    #endif
#endif

/* ========================================================================
 * ИНФОРМАЦИЯ О КОНФИГУРАЦИИ (для отладки)
 * ======================================================================== */
#ifdef SABER_CONFIG_DEFAULT
    #define SABER_CONFIG_NAME "DEFAULT"
#elif defined(SABER_CONFIG_FAST)
    #define SABER_CONFIG_NAME "FAST"
#elif defined(SABER_CONFIG_GOST)
    #define SABER_CONFIG_NAME "GOST"
#elif defined(SABER_CONFIG_GOST_FAST)
    #define SABER_CONFIG_NAME "GOST_FAST"
#elif defined(SABER_CONFIG_TEST)
    #define SABER_CONFIG_NAME "TEST"
#endif

#endif /* SABER_CONFIG_H */
