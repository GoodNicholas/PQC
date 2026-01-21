/**
 * @file pack_unpack_fast.h
 * @brief Wrapper для условного использования NEON-оптимизированных pack/unpack в FAST конфигурации
 */

#ifndef PACK_UNPACK_FAST_H
#define PACK_UNPACK_FAST_H

#include "params.h"
#include <stdint.h>

/* Используем NEON версии только для FAST конфигурации на ARM64 */
#if defined(__ARM_NEON) && (defined(SABER_CONFIG_FAST) || defined(SABER_CONFIG_GOST_FAST))
    /* NEON-оптимизированные версии */
    #include "pack_unpack_neon.h"

    /* Переопределяем стандартные функции на NEON версии */
    #define POLVECq2BS POLVECq2BS_neon
    #define BS2POLVECq BS2POLVECq_neon
    #define POLVECp2BS POLVECp2BS_neon
    #define BS2POLVECp BS2POLVECp_neon

#else
    /* Стандартные референсные версии */
    #include "../external/saber_ref/pack_unpack.h"
#endif

#endif /* PACK_UNPACK_FAST_H */