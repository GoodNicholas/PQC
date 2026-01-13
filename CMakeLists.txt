cmake_minimum_required(VERSION 3.10)
project(SABER_GOST C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ========================================================================
# КОНФИГУРАЦИЯ
# ========================================================================

# Выбор конфигурации (по умолчанию DEFAULT)
set(SABER_CONFIG "DEFAULT" CACHE STRING "Saber configuration: DEFAULT|FAST|FAST_V2|GOST|GOST_FAST|TEST")
set_property(CACHE SABER_CONFIG PROPERTY STRINGS DEFAULT FAST FAST_V2 GOST GOST_FAST TEST)

message(STATUS "SABER Configuration: ${SABER_CONFIG}")

# Определение макроса конфигурации
add_definitions(-DSABER_CONFIG_${SABER_CONFIG})

# ========================================================================
# ОПРЕДЕЛЕНИЕ ПЛАТФОРМЫ
# ========================================================================

# Проверка поддержки NEON на ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64")
    set(IS_ARM64 ON)
    message(STATUS "Platform: ARM64 detected")

    # Включение NEON и агрессивных оптимизаций для ARM64
    if(NOT MSVC)
        add_compile_options(
            -march=armv8-a+simd+crypto
            -mtune=native
        )
    endif()
else()
    set(IS_ARM64 OFF)
    message(STATUS "Platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Отключение NEON оптимизаций если платформа не ARM64
if(NOT IS_ARM64)
    if(SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2" OR SABER_CONFIG STREQUAL "GOST_FAST")
        message(WARNING "NEON optimizations requested but ARM64 not detected. Using fallback implementations.")
    endif()
endif()

# ========================================================================
# ОПТИМИЗАЦИИ КОМПИЛЯЦИИ
# ========================================================================

# Включение Link-Time Optimization для Release сборок
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "Link-Time Optimization (LTO): Enabled")
    else()
        message(STATUS "Link-Time Optimization (LTO): Not supported - ${ipo_error}")
    endif()
endif()

# Агрессивные флаги оптимизации для FAST конфигураций
if(SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2" OR SABER_CONFIG STREQUAL "GOST_FAST")
    if(NOT MSVC)
        add_compile_options(
            -O3                      # Максимальная оптимизация
            -fomit-frame-pointer     # Убрать frame pointer для дополнительного регистра
            -funroll-loops           # Развернуть циклы
            -ffast-math              # Быстрая математика (безопасна для нашего случая)
            -finline-functions       # Агрессивный инлайнинг
        )
        message(STATUS "Aggressive optimization flags: Enabled")
    endif()
endif()

# ========================================================================
# ПУТИ ВКЛЮЧЕНИЯ
# ========================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../SABER/Reference_Implementation_KEM
)

# ========================================================================
# ИСХОДНЫЕ ФАЙЛЫ REFERENCE IMPLEMENTATION
# ========================================================================

# Для FAST конфигурации используем стандартную reference реализацию
# (NEON оптимизации применяются в ChaCha20 и poly_fast_v4_addon)
if(SABER_CONFIG STREQUAL "FAST" AND IS_ARM64)
    set(REFERENCE_SOURCES
        ../SABER/Reference_Implementation_KEM/cbd.c
        ../SABER/Reference_Implementation_KEM/fips202.c
        ../SABER/Reference_Implementation_KEM/pack_unpack.c
        ../SABER/Reference_Implementation_KEM/poly.c
        ../SABER/Reference_Implementation_KEM/SABER_indcpa.c
        ../SABER/Reference_Implementation_KEM/verify.c
    )
elseif(SABER_CONFIG STREQUAL "FAST_V4" AND IS_ARM64)
    # FAST_V4: Full neon-ntt integration with asymmetric multiplication
    # Uses external neon-ntt repository implementation
    set(REFERENCE_SOURCES
        ../neon-ntt/saber/scheme/cbd.c
        ../neon-ntt/saber/scheme/pack_unpack.c
        ../neon-ntt/saber/scheme/fips202x2.c
        ../SABER/Reference_Implementation_KEM/fips202.c
        ../SABER/Reference_Implementation_KEM/verify.c
    )
else()
    set(REFERENCE_SOURCES
        ../SABER/Reference_Implementation_KEM/cbd.c
        ../SABER/Reference_Implementation_KEM/fips202.c
        ../SABER/Reference_Implementation_KEM/pack_unpack.c
        ../SABER/Reference_Implementation_KEM/poly.c
        ../SABER/Reference_Implementation_KEM/SABER_indcpa.c
        ../SABER/Reference_Implementation_KEM/verify.c
    )
endif()

# ========================================================================
# ВЫБОР МОДУЛЕЙ В ЗАВИСИМОСТИ ОТ КОНФИГУРАЦИИ
# ========================================================================

# KEM (общий для всех конфигураций)
set(KEM_SOURCES
    src/kem.c
)

# Core (различается для FAST_V4)
if(SABER_CONFIG STREQUAL "FAST_V4" AND IS_ARM64)
    # FAST_V4 использует neon-ntt SABER_indcpa.c напрямую + thin wrapper
    set(CORE_SOURCES
        ../neon-ntt/saber/scheme/SABER_indcpa.c
        src/kem/core_neon_ntt_wrapper.c  # Thin wrapper для SABER_GOST API
    )
else()
    # Остальные конфигурации используют стандартный core.c
    set(CORE_SOURCES src/core/core.c)
endif()

# Выбор Hash модуля
if(SABER_CONFIG STREQUAL "DEFAULT" OR SABER_CONFIG STREQUAL "TEST" OR SABER_CONFIG STREQUAL "FAST_V3" OR SABER_CONFIG STREQUAL "FAST_V4")
    # DEFAULT должна быть чистой референсной реализацией без оптимизаций
    # FAST_V3 тоже использует reference SHA-3 (SHAKE×3 NEON медленнее на Apple Silicon)
    # FAST_V4 использует reference SHA-3 для совместимости с neon-ntt
    set(HASH_SOURCES src/hash/hash_sha3.c)
    message(STATUS "Hash module: SHA-3 (reference)")
elseif(SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2")
    # FAST uses reference SHA-3 (Keccak×3 NEON not yet implemented)
    # Optimizations come from ChaCha20 NEON and poly operations
    set(HASH_SOURCES src/hash/hash_sha3.c)
    message(STATUS "Hash module: SHA-3 (reference)")
elseif(SABER_CONFIG STREQUAL "GOST")
    # GOST должна быть чистой реализацией без NEON оптимизаций
    set(HASH_SOURCES src/hash/hash_gost.c)
    message(STATUS "Hash module: GOST (Streebog)")
elseif(SABER_CONFIG STREQUAL "GOST_FAST")
    # GOST_FAST с NEON оптимизациями где возможно
    set(HASH_SOURCES
        src/hash/hash_gost.c
        src/hash/keccak_times3_neon.c
        # gen_matrix_shake3.c disabled for now
    )
    message(STATUS "Hash module: GOST (Streebog) + Keccak×3 NEON")
endif()

# Выбор RNG модуля
if(SABER_CONFIG STREQUAL "DEFAULT" OR SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2" OR SABER_CONFIG STREQUAL "FAST_V3" OR SABER_CONFIG STREQUAL "FAST_V4")
    # System RNG быстрее ChaCha20 NEON на Apple Silicon (использует аппаратное ускорение)
    set(RNG_SOURCES src/rng/rng_system.c)
    message(STATUS "RNG module: System RNG")
elseif(SABER_CONFIG STREQUAL "GOST" OR SABER_CONFIG STREQUAL "GOST_FAST")
    set(RNG_SOURCES src/rng/rng_gost_ctr.c)
    message(STATUS "RNG module: GOST CTR (Kuznyechik)")
elseif(SABER_CONFIG STREQUAL "TEST")
    set(RNG_SOURCES src/rng/rng_ctr_drbg.c)
    message(STATUS "RNG module: CTR_DRBG")
endif()

# Выбор Poly модуля
if(SABER_CONFIG STREQUAL "DEFAULT" OR SABER_CONFIG STREQUAL "GOST" OR SABER_CONFIG STREQUAL "TEST")
    set(POLY_SOURCES src/poly/poly_toom.c)
    message(STATUS "Poly module: Toom-Cook")
elseif(SABER_CONFIG STREQUAL "FAST" AND IS_ARM64)
    # FAST использует poly_ntt_neon.c - Incomplete-NTT NEON оптимизация
    set(POLY_SOURCES src/poly/poly_ntt_neon.c)
    message(STATUS "Poly module: NTT-Incomplete NEON")
elseif(SABER_CONFIG STREQUAL "FAST_V2" AND IS_ARM64)
    # FAST_V2 использует базовый neon-ntt (TCHES 2022) без asymmetric multiplication
    # asymmetric_mul несовместим с архитектурой SABER_GOST (см. FINDINGS.md)
    set(POLY_SOURCES
        src/poly/poly_ntt_simple.c
        # Assembly files from neon-ntt (базовый набор для NTT/iNTT)
        ../neon-ntt/saber/scheme/__asm_NTT.S
        ../neon-ntt/saber/scheme/__asm_iNTT.S
        ../neon-ntt/saber/scheme/__asm_mul.S
        ../neon-ntt/saber/scheme/__asm_pack_unpack.S
    )
    message(STATUS "Poly module: neon-ntt basic (TCHES 2022 - poly_mul only)")
elseif(SABER_CONFIG STREQUAL "FAST_V3" AND IS_ARM64)
    # FAST_V3: чистый neon-ntt БЕЗ SHAKE×3 NEON overhead
    # Только NTT оптимизация, остальное reference
    set(POLY_SOURCES
        src/poly/poly_ntt_simple.c
        ../neon-ntt/saber/scheme/__asm_NTT.S
        ../neon-ntt/saber/scheme/__asm_iNTT.S
        ../neon-ntt/saber/scheme/__asm_mul.S
        ../neon-ntt/saber/scheme/__asm_pack_unpack.S
    )
    message(STATUS "Poly module: neon-ntt basic (pure NTT, no SHAKE×3 overhead)")
elseif(SABER_CONFIG STREQUAL "FAST_V4" AND IS_ARM64)
    # FAST_V4: Full neon-ntt with asymmetric multiplication
    # Using complete neon-ntt implementation from external repository
    # Need GenMatrix for hash_sha3.c (can't use full poly.c due to missing poly_mul_acc)
    set(POLY_SOURCES
        src/poly/poly_genmatrix.c  # Minimal GenMatrix-only implementation
        # neon-ntt assembly files (all operations)
        ../neon-ntt/saber/scheme/__asm_NTT.S
        ../neon-ntt/saber/scheme/__asm_iNTT.S
        ../neon-ntt/saber/scheme/__asm_mul.S
        ../neon-ntt/saber/scheme/__asm_pack_unpack.S
        ../neon-ntt/saber/scheme/__asm_narrow.S
        # Keccak f1600x2 for shake128x2
        ../neon-ntt/saber/scheme/feat.S
    )
    message(STATUS "Poly module: neon-ntt asymmetric multiplication (full) + GenMatrix")
elseif(SABER_CONFIG STREQUAL "GOST_FAST" AND IS_ARM64)
    set(POLY_SOURCES src/poly/poly_ntt_neon.c)
    # Для GOST_FAST можно оставить AMX опционально
    if(APPLE AND USE_AMX)
        list(APPEND POLY_SOURCES src/poly/poly_amx.c)
        message(STATUS "Poly module: NTT-Incomplete NEON + Apple AMX")
    else()
        message(STATUS "Poly module: NTT-Incomplete NEON")
    endif()
elseif((SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2" OR SABER_CONFIG STREQUAL "GOST_FAST") AND NOT IS_ARM64)
    set(POLY_SOURCES src/poly/poly_toom.c)
    message(STATUS "Poly module: Toom-Cook (fallback)")
endif()

# Выбор FO Utils модуля
if(SABER_CONFIG STREQUAL "DEFAULT" OR SABER_CONFIG STREQUAL "TEST" OR SABER_CONFIG STREQUAL "FAST_V3" OR SABER_CONFIG STREQUAL "FAST_V4")
    # FAST_V3 и FAST_V4 используют reference FO utils (ChaCha20 NEON может добавлять overhead)
    set(FO_UTILS_SOURCES src/fo_utils/fo_utils_ref.c)
    message(STATUS "FO Utils module: Reference")
elseif((SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2") AND IS_ARM64)
    set(FO_UTILS_SOURCES src/fo_utils/fo_utils_chacha_neon.c)
    message(STATUS "FO Utils module: ChaCha20 NEON")
elseif((SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2") AND NOT IS_ARM64)
    set(FO_UTILS_SOURCES src/fo_utils/fo_utils_ref.c)
    message(STATUS "FO Utils module: Reference (fallback)")
elseif(SABER_CONFIG STREQUAL "GOST" OR SABER_CONFIG STREQUAL "GOST_FAST")
    set(FO_UTILS_SOURCES src/fo_utils/fo_utils_batch.c)
    message(STATUS "FO Utils module: Batch")
endif()

# ========================================================================
# БИБЛИОТЕКА SABER_GOST
# ========================================================================

# Дополнительные NEON оптимизации только для FAST конфигураций
set(NEON_OPTIMIZATION_SOURCES "")
if(IS_ARM64)
    if(SABER_CONFIG STREQUAL "FAST" OR SABER_CONFIG STREQUAL "FAST_V2")
        # Для FAST используется poly_ntt_neon.c с встроенными NEON оптимизациями
        # pack/unpack NEON оптимизации уже включены в poly_ntt_neon.c
        message(STATUS "FAST: Using poly_ntt_neon with built-in NEON optimizations")
    elseif(SABER_CONFIG STREQUAL "GOST_FAST")
        set(NEON_OPTIMIZATION_SOURCES
            src/pack_unpack_neon.c
            src/cbd_neon.c
        )
        message(STATUS "GOST_FAST: Additional NEON optimizations enabled")
    endif()
endif()

# Batching support (optional)
option(ENABLE_BATCHING "Enable 2x parallel batching with NEON" OFF)
set(BATCH_SOURCES "")
if(ENABLE_BATCHING AND IS_ARM64)
    set(BATCH_SOURCES src/batch/batch_kem.c)
    message(STATUS "Batching: Enabled (2x parallel with NEON)")
    add_definitions(-DSABER_BATCHING_ENABLED)
endif()

add_library(saber_gost STATIC
    ${REFERENCE_SOURCES}
    ${KEM_SOURCES}
    ${CORE_SOURCES}
    ${HASH_SOURCES}
    ${RNG_SOURCES}
    ${POLY_SOURCES}
    ${FO_UTILS_SOURCES}
    ${NEON_OPTIMIZATION_SOURCES}
    ${BATCH_SOURCES}
)

target_include_directories(saber_gost PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Include neon-ntt headers for FAST_V2, FAST_V3, and FAST_V4
if((SABER_CONFIG STREQUAL "FAST_V2" OR SABER_CONFIG STREQUAL "FAST_V3" OR SABER_CONFIG STREQUAL "FAST_V4") AND IS_ARM64)
    target_include_directories(saber_gost PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../neon-ntt/saber/scheme
        ${CMAKE_CURRENT_SOURCE_DIR}/../neon-ntt/ntt
    )
endif()

# Link with Accelerate framework only for GOST_FAST if AMX is enabled
if(APPLE AND SABER_CONFIG STREQUAL "GOST_FAST" AND USE_AMX)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        target_link_libraries(saber_gost PUBLIC ${ACCELERATE_FRAMEWORK})
        message(STATUS "Apple Accelerate framework: Linked for AMX support")
    else()
        message(STATUS "Apple Accelerate framework: Not found")
    endif()
endif()

# ========================================================================
# ВНЕШНИЕ ЗАВИСИМОСТИ
# ========================================================================

# Для GOST конфигураций: компиляция необходимых файлов из engine/
if(SABER_CONFIG STREQUAL "GOST" OR SABER_CONFIG STREQUAL "GOST_FAST")
    # Путь к libgost (предполагается что engine/ находится в ../engine)
    target_include_directories(saber_gost PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine
    )

    # Добавляем исходные файлы Streebog из engine/
    target_sources(saber_gost PRIVATE
        ../engine/gosthash2012.c
        ../engine/gosthash2012_const.h
        ../engine/gosthash2012_precalc.h
        ../engine/gosthash2012_ref.h
        ../engine/gosthash2012_sse2.h
    )

    message(STATUS "GOST engine: Streebog sources included")
endif()

# ChaCha20 NEON: используем встроенную реализацию (не требуется libsodium)
# Для интеграции с libsodium в production, раскомментируйте код ниже:
#
# if(SABER_CONFIG STREQUAL "FAST" AND IS_ARM64)
#     find_package(PkgConfig)
#     if(PkgConfig_FOUND)
#         pkg_check_modules(LIBSODIUM libsodium)
#         if(LIBSODIUM_FOUND)
#             target_link_directories(saber_gost PRIVATE ${LIBSODIUM_LIBRARY_DIRS})
#             target_link_libraries(saber_gost ${LIBSODIUM_LIBRARIES})
#             message(STATUS "libsodium found: ${LIBSODIUM_VERSION}")
#         endif()
#     endif()
# endif()
if(SABER_CONFIG STREQUAL "FAST")
    message(STATUS "ChaCha20: Using built-in implementation")
endif()

# Для CTR_DRBG: линковка с OpenSSL
if(SABER_CONFIG STREQUAL "TEST")
    find_package(OpenSSL REQUIRED)
    target_link_libraries(saber_gost OpenSSL::Crypto)
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
endif()

# ========================================================================
# ОПЦИИ КОМПИЛЯЦИИ
# ========================================================================

# Оптимизация
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT MSVC)
        target_compile_options(saber_gost PRIVATE -O3 -Wall -Wextra)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT MSVC)
        target_compile_options(saber_gost PRIVATE -g -O0 -Wall -Wextra -Wpedantic)
    endif()
endif()

# ========================================================================
# ТЕСТЫ И БЕНЧМАРКИ (опционально)
# ========================================================================

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

if(BUILD_TESTS)
    enable_testing()

    # Базовый unit test
    add_executable(test_basic tests/test_basic.c)
    target_link_libraries(test_basic saber_gost)
    add_test(NAME test_basic COMMAND test_basic)

    # Pack/Unpack NEON test
    add_executable(test_pack_unpack tests/test_pack_unpack_neon.c)
    target_link_libraries(test_pack_unpack saber_gost)
    add_test(NAME test_pack_unpack COMMAND test_pack_unpack)

    # CBD NEON test
    add_executable(test_cbd tests/test_cbd_neon.c)
    target_link_libraries(test_cbd saber_gost)
    add_test(NAME test_cbd COMMAND test_cbd)

    # NTT correctness test
    add_executable(test_ntt_correctness test_ntt_correctness.c src/poly/poly_ntt_neon_test.c)
    target_link_libraries(test_ntt_correctness saber_gost)
    add_test(NAME test_ntt_correctness COMMAND test_ntt_correctness)

    # Batch KEM test (if batching is enabled)
    if(ENABLE_BATCHING AND IS_ARM64)
        add_executable(test_batch_kem tests/test_batch_kem.c)
        target_link_libraries(test_batch_kem saber_gost)
        add_test(NAME test_batch_kem COMMAND test_batch_kem)
        message(STATUS "Batch KEM test: Enabled")
    endif()

    # Polynomial correctness test (neon-ntt vs Toom-Cook)
    if(SABER_CONFIG STREQUAL "FAST_V2" AND IS_ARM64)
        add_executable(test_poly_correctness tests/test_poly_correctness.c)
        target_link_libraries(test_poly_correctness saber_gost)
        add_test(NAME test_poly_correctness COMMAND test_poly_correctness)

        # NTT values diagnostic test
        add_executable(test_ntt_values tests/test_ntt_values.c)
        target_link_libraries(test_ntt_values saber_gost)

        # Asymmetric mul debug test
        add_executable(test_asymmetric_debug tests/test_asymmetric_debug.c)
        target_link_libraries(test_asymmetric_debug saber_gost)
        target_include_directories(test_asymmetric_debug PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../neon-ntt/saber/scheme
        )
    endif()

    # Debug test for FAST_V4
    if(SABER_CONFIG STREQUAL "FAST_V4" AND IS_ARM64)
        add_executable(debug_neon_ntt tests/debug_neon_ntt.c)
        target_link_libraries(debug_neon_ntt saber_gost)

        add_executable(debug_indcpa tests/debug_indcpa.c)
        target_link_libraries(debug_indcpa saber_gost)
        target_include_directories(debug_indcpa PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../neon-ntt/saber/scheme
        )

        add_executable(debug_kem_detailed tests/debug_kem_detailed.c)
        target_link_libraries(debug_kem_detailed saber_gost)

        add_executable(bench_indcpa tests/bench_indcpa.c)
        target_link_libraries(bench_indcpa saber_gost)
        target_include_directories(bench_indcpa PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../neon-ntt/saber/scheme
        )
    endif()

    message(STATUS "Tests enabled")
endif()

if(BUILD_BENCHMARKS)
    # Benchmark suite
    add_executable(benchmark tests/benchmark.c)
    target_link_libraries(benchmark saber_gost)

    # Detailed profiling benchmark
    add_executable(benchmark_detailed tests/benchmark_detailed.c)
    target_link_libraries(benchmark_detailed saber_gost)

    # Micro benchmark
    add_executable(bench_micro tests/bench_micro.c)
    target_link_libraries(bench_micro saber_gost)

    message(STATUS "Benchmarks enabled")
endif()

# ========================================================================
# ИНФОРМАЦИЯ О СБОРКЕ
# ========================================================================

message(STATUS "==================================================")
message(STATUS "SABER_GOST Configuration Summary:")
message(STATUS "  Configuration: ${SABER_CONFIG}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  ARM64/NEON: ${IS_ARM64}")
message(STATUS "  Hash: ${HASH_SOURCES}")
message(STATUS "  RNG: ${RNG_SOURCES}")
message(STATUS "  Poly: ${POLY_SOURCES}")
message(STATUS "  FO Utils: ${FO_UTILS_SOURCES}")
message(STATUS "==================================================")

# Neon NTT test (TCHES 2022)
if(BUILD_TESTS AND IS_ARM64)
    add_executable(test_neon_ntt
        test_neon_ntt.c
        src/poly/test/poly_ntt_neon.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_narrow.S
        src/poly/test/asm_pack_unpack.S
        ../SABER/Reference_Implementation_KEM/poly_mul.c
    )
    target_include_directories(test_neon_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
        ${CMAKE_CURRENT_SOURCE_DIR}/../SABER/Reference_Implementation_KEM
    )
    add_test(NAME test_neon_ntt COMMAND test_neon_ntt)

    # Debug NTT
    add_executable(debug_ntt
        debug_ntt.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
    )
    target_include_directories(debug_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Detailed trace NTT
    add_executable(trace_ntt_detailed
        trace_ntt_detailed.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
    )
    target_include_directories(trace_ntt_detailed PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Direct NTT test
    add_executable(test_ntt_direct
        test_ntt_direct.c
        src/poly/test/ntt.S
    )
    target_include_directories(test_ntt_direct PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Position NTT test
    add_executable(test_ntt_positions
        test_ntt_positions.c
        src/poly/test/ntt.S
    )
    target_include_directories(test_ntt_positions PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Dense polynomial test
    add_executable(test_dense_poly
        test_dense_poly.c
        src/poly/test/poly_ntt_neon.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_narrow.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_dense_poly PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Packed NTT test (ПРАВИЛЬНАЯ версия с упаковкой)
    add_executable(test_packed_ntt
        test_packed_ntt.c
        src/poly/test/poly_ntt_neon_packed.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_narrow.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_packed_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Debug packed
    add_executable(debug_packed
        debug_packed.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
    )
    target_include_directories(debug_packed PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Final NTT test with Montgomery
    add_executable(test_final_ntt
        test_final_ntt.c
        src/poly/test/poly_ntt_neon.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_narrow.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_final_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Debug Montgomery
    add_executable(debug_montgomery
        debug_montgomery.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
    )
    target_include_directories(debug_montgomery PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Simple NTT test (no packing)
    add_executable(test_simple_ntt
        test_simple_ntt.c
        src/poly/test/poly_ntt_neon_simple.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
    )
    target_include_directories(test_simple_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # CORRECT NTT test with __asm_16_to_32
    add_executable(test_correct_ntt
        test_correct_ntt.c
        src/poly/test/poly_ntt_neon_correct.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_correct_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Test packed NTT
    add_executable(test_ntt_packed
        test_ntt_packed.c
        src/poly/test/poly_ntt_packed.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_pack_unpack.S
        src/poly/test/asm_narrow.S
    )
    target_include_directories(test_ntt_packed PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Trace NTT test
    add_executable(test_trace_ntt
        test_trace_ntt.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_trace_ntt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # Fixed NTT test
    add_executable(test_ntt_fixed
        test_ntt_fixed.c
        src/poly/test/poly_ntt_fixed.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_ntt_fixed PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # FAST NTT test
    add_executable(test_ntt_fast
        test_ntt_fast.c
        src/poly/poly_ntt_fast.c
        src/poly/test/ntt.S
        src/poly/test/intt.S
        src/poly/test/polymul.S
        src/poly/test/asm_pack_unpack.S
    )
    target_include_directories(test_ntt_fast PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/test
    )

    # CORRECT NTT test with original neon-ntt files
    add_executable(test_ntt_correct
        test_ntt_correct.c
        src/poly/poly_mul_ntt_correct.c
        src/poly/ntt_correct/__asm_NTT.S
        src/poly/ntt_correct/__asm_iNTT.S
        src/poly/ntt_correct/__asm_mul.S
        src/poly/ntt_correct/__asm_pack_unpack.S
        src/poly/ntt_correct/__asm_narrow.S
    )
    target_include_directories(test_ntt_correct PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/poly/ntt_correct
    )

    message(STATUS "Neon NTT test: Enabled")
endif()

# ==============================================================================
# Experimental Benchmark for Performance Study
# ==============================================================================
add_executable(experiment_benchmark
    experiment_benchmark.c
)

target_link_libraries(experiment_benchmark saber_gost)

target_include_directories(experiment_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

message(STATUS "Experimental benchmark: Enabled")
