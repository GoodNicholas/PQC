#ifndef POLY_AMX_H
#define POLY_AMX_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__

/**
 * Apple AMX-accelerated polynomial operations
 *
 * These functions use Apple's Accelerate framework to leverage
 * the AMX matrix coprocessor for significant performance improvements.
 *
 * Expected speedups (based on research):
 * - poly_mul: ~2-3x faster than NEON
 * - matrix_vector_mul: ~2.5x faster (151% improvement)
 * - inner_product: ~2.2x faster
 * - Overall KEM: ~13% improvement
 */

/* Polynomial multiplication using AMX */
void poly_mul_amx(uint16_t r[SABER_N],
                  const uint16_t a[SABER_N],
                  const uint16_t b[SABER_N]);

/* Matrix-vector multiplication using AMX */
void matrix_vector_mul_amx(uint16_t r[SABER_L][SABER_N],
                           const uint16_t A[SABER_L][SABER_L][SABER_N],
                           const uint16_t s[SABER_L][SABER_N],
                           int transpose);

/* Inner product using AMX */
void inner_product_amx(uint16_t r[SABER_N],
                      const uint16_t a[SABER_L][SABER_N],
                      const uint16_t b[SABER_L][SABER_N]);

/* Check if AMX is available at runtime */
int is_amx_available(void);

#endif /* __APPLE__ */

#ifdef __cplusplus
}
#endif

#endif /* POLY_AMX_H */