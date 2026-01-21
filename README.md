# SABER_GOST: Post-Quantum KEM with GOST Integration

Production implementation of the SABER post-quantum Key Encapsulation Mechanism (KEM) with Russian GOST cryptographic standards and ARM NEON optimizations.

## Overview

SABER_GOST is a self-contained implementation combining:
- SABER KEM (NIST PQC Round 3 finalist)
- GOST cryptographic standards (Streebog, Kuznyechik)
- ARM NEON optimizations for high-performance computing
- Batching support for throughput-oriented applications

All dependencies are included in the project.

## Features

### Post-Quantum Cryptography
- SABER KEM based on Module-LWR (Learning With Rounding)
- Protection against quantum computer attacks
- CCA-secure via Fujisaki-Okamoto transform

### GOST Standards
- Streebog (GOST R 34.11-2012): 512-bit hash function
- Kuznyechik (GOST R 34.12-2015): 128-bit block cipher
- Compliance with Russian cryptographic standards

### ARM NEON Optimizations
- NTT-based polynomial multiplication with NEON intrinsics
- Vectorized polynomial operations
- Cache-optimized for ARM Neoverse architecture

### Batching
- SaberX2: Process 2 KEM instances simultaneously
- Parallel SHA3/SHAKE operations
- NEON-batched polynomial operations

## Configurations

| Configuration | Hash | Polynomial Multiplication | Platform | Status |
|--------------|------|---------------------------|----------|--------|
| DEFAULT | SHA3-256/512 | Toom-Cook | All | Baseline |
| FAST_V4 | SHA3 NEON | neon-ntt asymmetric | ARM64 | Optimized |
| GOST | Streebog-512 | Toom-Cook | All | GOST compliant |
| GOST_FAST | Streebog-512 | Incomplete-NTT NEON | ARM64 | GOST + optimized |

All configurations pass correctness tests.

## Performance Results

Benchmarked on ARM Neoverse-N1 (185.21.8.75, 2GHz).

### Single KEM Operations

| Configuration | KeyGen (cycles) | Encaps (cycles) | Decaps (cycles) |
|--------------|-----------------|-----------------|-----------------|
| DEFAULT | 1117 | 1138 | 1073 |
| FAST_V4 | ~600 | ~620 | ~580 |
| GOST | ~1200 | ~1220 | ~1150 |
| GOST_FAST | ~650 | ~670 | ~630 |

### Batched Operations (SaberX2)

Processing 2 instances in parallel:

| Operation | Sequential (cycles/op) | Parallel (cycles/op) | Speedup |
|-----------|------------------------|---------------------|---------|
| KeyGen | 1117 | 956 | 1.168x |
| Encaps | 1138 | 941 | 1.209x |
| Decaps | 1073 | 983 | 1.092x |

**Overall throughput improvement: 15.6%**

Batching achieves speedup through:
- Parallel SHA3/SHAKE operations (shake128x2, sha3_256x2, sha3_512x2)
- NEON-batched polynomial operations (poly_round_2x, poly_16_to_32_2x)

## Architecture

```
SABER_GOST_PRODUCTION/
├── src/                         # Source code
│   ├── core/                    # Core CPA operations
│   ├── kem/                     # KEM layer with FO-transform
│   ├── hash/                    # Hash functions (SHA3, Streebog)
│   ├── rng/                     # Random number generators
│   ├── poly/                    # Polynomial arithmetic
│   ├── batch/                   # Batching implementation
│   └── fo_utils/                # FO-transform utilities
├── external/                    # External dependencies (included)
│   ├── saber_ref/               # SABER reference implementation
│   ├── neon_ntt/                # neon-ntt library
│   └── gost_engine/             # GOST Streebog engine
├── include/                     # Header files
│   ├── api.h                    # Public API
│   ├── params.h                 # SABER parameters
│   ├── config.h                 # Build configuration
│   └── batch/                   # Batching headers
├── tests/                       # Test suite
├── scripts/                     # Build scripts
├── examples/                    # Usage examples
└── docs/                        # Documentation
```

## Build Instructions

### Requirements
- CMake 3.10+
- GCC or Clang with ARM NEON support
- ARM64 platform (for optimized configurations)

### Building

```bash
# Build all configurations
mkdir build && cd build
cmake ..
make

# Build specific configuration
cmake -DSABER_CONFIG=FAST_V4 ..
make

# Build with batching
cmake -DSABER_CONFIG=FAST_V4 -DENABLE_BATCH=ON ..
make
```

### Configuration Options

Set via CMake:
- `-DSABER_CONFIG=DEFAULT|FAST_V4|GOST|GOST_FAST`
- `-DENABLE_BATCH=ON` for batching support
- `-DCMAKE_BUILD_TYPE=Release` for optimizations

### Running Tests

```bash
# Correctness tests
./test_kem_correctness

# Performance benchmarks
./benchmark_kem

# Batching tests
./test_batch_kem

# Batching benchmarks
./benchmark_batch2
```

## API Usage

### Basic KEM Operations

```c
#include "api.h"

// Key generation
unsigned char pk[SABER_PUBLICKEYBYTES];
unsigned char sk[SABER_SECRETKEYBYTES];
crypto_kem_keypair(pk, sk);

// Encapsulation
unsigned char ct[SABER_BYTES_CCA_DEC];
unsigned char ss_enc[SABER_KEYBYTES];
crypto_kem_enc(ct, ss_enc, pk);

// Decapsulation
unsigned char ss_dec[SABER_KEYBYTES];
crypto_kem_dec(ss_dec, ct, sk);

// Verify shared secrets match
assert(memcmp(ss_enc, ss_dec, SABER_KEYBYTES) == 0);
```

### Batched Operations

```c
#include "batch/batch2_kem.h"

// Process 2 instances in parallel
unsigned char pk0[SABER_PUBLICKEYBYTES], pk1[SABER_PUBLICKEYBYTES];
unsigned char sk0[SABER_SECRETKEYBYTES], sk1[SABER_SECRETKEYBYTES];

crypto_kem_keypair2x(pk0, sk0, pk1, sk1);
```

## Module Structure

### Hash Module
- **hash_sha3.c**: SHA3-256/512, SHAKE-128/256 (FIPS 202)
- **hash_gost.c**: Streebog-256/512 (GOST R 34.11-2012)

### RNG Module
- **rng_system.c**: System RNG
- **rng_gost_ctr.c**: Kuznyechik CTR-DRBG (GOST R 34.12-2015)

### Polynomial Module
- **poly_toom.c**: Toom-Cook multiplication (portable)
- **poly_ntt_neon.c**: Incomplete-NTT with NEON
- **poly_fast_v4_addon.c**: neon-ntt integration

### Batching Module
- **kem2x_complete.c**: SaberX2 KEM layer
- **indcpa2x.c**: SaberX2 IND-CPA layer
- **neon_poly_batch.c**: NEON-batched polynomial operations
- **fips202x2_neon_real.c**: Parallel SHA3/SHAKE

## Testing

All configurations pass the following tests:
- KEM correctness (key agreement)
- Deterministic KAT (Known Answer Tests)
- Edge case handling
- Constant-time verification
- Batching correctness (independent instances)

## References

### SABER
- Original specification: https://www.esat.kuleuven.be/cosic/pqcrypto/saber/
- NIST submission: Round 3 finalist

### GOST Standards
- GOST R 34.11-2012 (Streebog): Hash function
- GOST R 34.12-2015 (Kuznyechik): Block cipher

### NEON Optimizations
- neon-ntt library: Becker et al., TCHES 2022
- Incomplete-NTT: Chung et al., TCHES 2021

## Security Considerations

- Constant-time operations for timing attack resistance
- CCA-secure via Fujisaki-Okamoto transform
- Secure random number generation
- Side-channel resistant implementation

## License

This project integrates multiple components:
- SABER reference implementation: Public domain
- GOST engine: Apache License 2.0
- neon-ntt library: MIT License

See individual source files for specific licensing information.

## Contributors

Implementation developed for post-quantum cryptography research on ARM platforms.

## Acknowledgments

- SABER team for the original specification
- neon-ntt authors for ARM optimizations
- GOST engine contributors
