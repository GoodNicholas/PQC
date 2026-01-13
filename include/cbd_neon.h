#ifndef CBD_NEON_H
#define CBD_NEON_H

#include <stdint.h>

/* NEON-optimized CBD functions */
void cbd_neon(uint16_t s[256], const uint8_t buf[256]);
void cbd_neon_vectorized(uint16_t s[256], const uint8_t buf[256]);

#endif /* CBD_NEON_H */