#ifndef SABER_POLY_H
#define SABER_POLY_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file poly.h
 * @brief Интерфейс модуля полиномиальных операций для SABER_GOST
 *
 * Поддерживает различные методы умножения полиномов:
 * - Toom-Cook 4-way (DEFAULT, GOST, TEST) - классический метод
 * - NTT-Incomplete NEON (FAST, GOST_FAST) - оптимизированное умножение через NTT
 *
 * Все полиномы работают в кольце Z_q[x]/(x^256 + 1), где q = 8192.
 */

/* ========================================================================
 * УМНОЖЕНИЕ ПОЛИНОМОВ
 * ======================================================================== */

/**
 * @brief poly_mul - умножение двух полиномов
 *
 * Вычисляет r = a * b (mod x^256 + 1) в кольце Z_8192[x].
 *
 * @param[out] r  Результат умножения [SABER_N]
 * @param[in]  a  Первый полином [SABER_N]
 * @param[in]  b  Второй полином [SABER_N]
 */
void poly_mul(uint16_t r[SABER_N],
              const uint16_t a[SABER_N],
              const uint16_t b[SABER_N]);

/* ========================================================================
 * МАТРИЧНО-ВЕКТОРНЫЕ ОПЕРАЦИИ
 * ======================================================================== */

/**
 * @brief matrix_vector_mul - умножение матрицы на вектор
 *
 * Вычисляет r = A * s (или r = A^T * s если transpose=1).
 * A - матрица полиномов размером [SABER_L][SABER_L][SABER_N]
 * s - вектор полиномов размером [SABER_L][SABER_N]
 * r - результирующий вектор полиномов размером [SABER_L][SABER_N]
 *
 * Каждый элемент результата:
 * r[i] = sum_{j=0}^{SABER_L-1} A[i][j] * s[j]  (если transpose=0)
 * r[i] = sum_{j=0}^{SABER_L-1} A[j][i] * s[j]  (если transpose=1)
 *
 * Оптимизации:
 * - Toom-Cook: последовательное умножение полиномов
 * - NTT-Incomplete NEON: параллельное умножение в NTT-домене (ускорение ~1.7×)
 *
 * @param[out] r         Результирующий вектор [SABER_L][SABER_N]
 * @param[in]  A         Матрица [SABER_L][SABER_L][SABER_N]
 * @param[in]  s         Входной вектор [SABER_L][SABER_N]
 * @param[in]  transpose Флаг транспонирования (0 = A*s, 1 = A^T*s)
 */
void matrix_vector_mul(uint16_t r[SABER_L][SABER_N],
                       const uint16_t A[SABER_L][SABER_L][SABER_N],
                       const uint16_t s[SABER_L][SABER_N],
                       int transpose);

/**
 * @brief inner_product - скалярное произведение двух векторов полиномов
 *
 * Вычисляет r = <a, b> = sum_{i=0}^{SABER_L-1} a[i] * b[i].
 * a, b - векторы полиномов размером [SABER_L][SABER_N]
 * r - результирующий полином размером [SABER_N]
 *
 * @param[out] r  Результирующий полином [SABER_N]
 * @param[in]  a  Первый вектор [SABER_L][SABER_N]
 * @param[in]  b  Второй вектор [SABER_L][SABER_N]
 */
void inner_product(uint16_t r[SABER_N],
                   const uint16_t a[SABER_L][SABER_N],
                   const uint16_t b[SABER_L][SABER_N]);

#ifdef __cplusplus
}
#endif

#endif /* SABER_POLY_H */
