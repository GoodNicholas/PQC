/**
 * @file rng_system.c
 * @brief System RNG implementation for SABER_GOST
 *
 * Uses system's cryptographically secure random number generator.
 * On macOS/BSD: arc4random_buf
 * On Linux: getrandom() or /dev/urandom
 *
 * This is the default RNG for DEFAULT configuration.
 */

#include "../../include/rng.h"
#include "../../include/config.h"
#include <stdlib.h>

/* Platform-specific RNG */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    #include <stdlib.h>
    #define HAVE_ARC4RANDOM
#elif defined(__linux__)
    #include <sys/random.h>
    #define HAVE_GETRANDOM
#else
    #include <stdio.h>
    #define HAVE_DEV_URANDOM
#endif

/* ========================================================================
 * ГЕНЕРАЦИЯ СЛУЧАЙНЫХ БАЙТ
 * ======================================================================== */

/**
 * random_bytes - генерация случайных байт через системный RNG
 *
 * Использует:
 * - macOS/BSD: arc4random_buf() (ChaCha20-based)
 * - Linux: getrandom() syscall
 * - Fallback: /dev/urandom
 */
void random_bytes(uint8_t *buf, size_t len)
{
#ifdef HAVE_ARC4RANDOM
    arc4random_buf(buf, len);

#elif defined(HAVE_GETRANDOM)
    ssize_t ret;
    size_t offset = 0;

    while (offset < len) {
        ret = getrandom(buf + offset, len - offset, 0);
        if (ret < 0) {
            /* Ошибка - критическая ситуация */
            abort();
        }
        offset += ret;
    }

#elif defined(HAVE_DEV_URANDOM)
    FILE *fp = fopen("/dev/urandom", "rb");
    if (!fp) {
        abort();
    }

    size_t nread = fread(buf, 1, len, fp);
    fclose(fp);

    if (nread != len) {
        abort();
    }

#else
    #error "No secure random number generator available"
#endif
}

/* ========================================================================
 * ИНИЦИАЛИЗАЦИЯ (пустая для system RNG)
 * ======================================================================== */

/**
 * rng_init - инициализация RNG
 *
 * Для system RNG ничего не делает, т.к. система сама управляет энтропией.
 * Функция существует для совместимости интерфейса с детерминированными RNG.
 */
void rng_init(const uint8_t *seed, size_t seedlen)
{
    /* System RNG не требует инициализации */
    (void)seed;
    (void)seedlen;
}

/* ========================================================================
 * СОВМЕСТИМОСТЬ С REFERENCE IMPLEMENTATION
 * ======================================================================== */

/**
 * randombytes - обертка для совместимости с SABER Reference Implementation
 *
 * SABER Reference Implementation ожидает функцию randombytes() для генерации
 * случайных байт в indcpa_kem_keypair и других функциях.
 */
int randombytes(unsigned char *x, unsigned long long xlen)
{
    random_bytes((uint8_t *)x, (size_t)xlen);
    return 0;
}
