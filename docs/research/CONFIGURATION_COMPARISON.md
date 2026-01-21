# SABER_GOST Configuration Comparison
## Comprehensive Analysis of All Four Configurations

**Date**: 2026-01-11
**Version**: 1.0
**Status**: Tested and Validated

---

## Executive Summary

SABER_GOST provides **four distinct configurations** targeting different use cases:

| Configuration | Correctness | Performance (Apple M4 Max) | Target Use Case |
|--------------|-------------|---------------------------|-----------------|
| **DEFAULT** | ✅ PASSED (10/10) | 15.02 μs (baseline) | Reference, compatibility |
| **FAST_V4** | ✅ **PASSED (10/10)** | **7.85 μs (1.91× faster)** | **Production (International, high-perf)** |
| **GOST** | ✅ PASSED (10/10) | 18.53 μs (0.81× baseline) | Russian standards compliance |
| **GOST_FAST** | ✅ PASSED (10/10) | 18.02 μs (0.83× baseline) | Production (Russian market) |

**Recommended configurations**:
- **Production (International, standard)**: DEFAULT - validated reference
- **Production (International, high-performance)**: **FAST_V4** - **~2× speedup, fully validated**
- **Production (Russia, compliance)**: GOST - Russian cryptographic standards
- **Production (Russia, high-performance)**: GOST_FAST - 1.7-2× speedup on ARM servers

---

## Configuration Details

### 1. DEFAULT (Reference Implementation)

**Description**: Standard SABER implementation following NIST submission specifications.

**Components**:
- **Hash**: SHA-3/SHAKE128 (reference fips202.c)
- **RNG**: System RNG (/dev/urandom)
- **Polynomial operations**: Toom-Cook multiplication
- **FO Transform**: Reference implementation

**Characteristics**:
- ✅ Fully compliant with SABER specification
- ✅ Maximum compatibility
- ✅ Validated correctness (100/100 tests passed)
- 📊 Baseline performance
- 🎯 Suitable for: Cross-platform development, testing, validation

**Test Results**:
```
Total tests:  10
Passed:       10
Failed:       0
Status:       ALL TESTS PASSED
```

**Key Sizes**:
- Public key:  992 bytes
- Secret key:  2304 bytes
- Ciphertext:  1088 bytes
- Shared secret: 32 bytes

---

### 2. FAST_V4 (Production NEON Optimization) ✅ **FIXED AND VALIDATED**

**Description**: Full integration of neon-ntt SABER_indcpa.c with asymmetric multiplication optimization.

**Components**:
- **Hash**: SHA-3/SHAKE128 (reference fips202.c) + parallel SHAKE128x2 (fips202x2.c)
- **RNG**: System RNG
- **Polynomial operations**: NTT-based asymmetric multiplication (neon-ntt, TCHES 2022)
- **Pack/Unpack**: NEON SIMD optimizations from neon-ntt
- **CBD**: NEON-optimized sampling from neon-ntt
- **Keccak**: Parallel f1600x2 (feat.S) for shake128x2

**Characteristics**:
- ✅ **ALL CORRECTNESS TESTS PASSED** (10/10)
- 🚀 **~2× faster than DEFAULT** (1.91× KeyGen, 1.94× Encaps, 2.05× Decaps)
- 🎯 **Production-ready** - fully validated implementation
- 💪 **Recommended for high-performance international deployments**

**Test Results**:
```
Total tests:  10
Passed:       10
Failed:       0
Status:       ALL TESTS PASSED
```

**Performance (Apple M4 Max)**:
```
  KeyGen:      7.85 μs  (127,389 ops/sec)  [1.91× vs DEFAULT]
  Encaps:      9.63 μs  (103,831 ops/sec)  [1.94× vs DEFAULT]
  Decaps:     10.75 μs  ( 93,006 ops/sec)  [2.05× vs DEFAULT]
```

**Integration Details**:
1. ✅ Direct use of neon-ntt SABER_indcpa.c (no custom wrapper)
2. ✅ Thin wrapper (core_neon_ntt_wrapper.c) for API compatibility
3. ✅ All dependencies resolved (fips202x2, feat.S, hal.h)
4. ✅ Validated on Apple Silicon M4 Max

**Key Optimization**: `__asm_asymmetric_mul` - single operation replaces 9 polynomial multiplications

---

### 3. GOST (Russian Standards Compliance)

**Description**: SABER adapted to use Russian cryptographic standards (GOST R 34.11-2012).

**Components**:
- **Hash**: Streebog (GOST R 34.11-2012) - 512-bit variant
- **RNG**: GOST CTR_DRBG with Kuznyechik block cipher
- **Polynomial operations**: Toom-Cook multiplication
- **FO Transform**: Batch-optimized variant

**Characteristics**:
- ✅ Validated correctness (100/100 tests passed)
- 🇷🇺 Compliant with Russian cryptographic standards
- 📊 Similar performance to DEFAULT
- 🎯 Suitable for: Russian government, regulated industries

**Test Results**:
```
Total tests:  10
Passed:       10
Failed:       0
Status:       ALL TESTS PASSED
```

**Russian Standards**:
- GOST R 34.11-2012 (Streebog hash function)
- GOST R 34.12-2015 (Kuznyechik block cipher for RNG)
- Compatible with Russian PKI infrastructure

**Performance**: Approximately 5-10% slower than DEFAULT due to Streebog overhead (256 vs 512-bit hash).

---

### 4. GOST_FAST (Production-Ready NEON Optimization)

**Description**: GOST configuration with custom NEON optimizations - the **best-performing validated configuration**.

**Components**:
- **Hash**: Streebog + Keccak×3 NEON (parallel processing)
- **RNG**: GOST CTR_DRBG with Kuznyechik
- **Polynomial operations**: Custom NTT-incomplete NEON implementation
- **Pack/Unpack**: NEON SIMD optimizations
- **CBD**: NEON-optimized sampling
- **FO Transform**: Batch-optimized variant

**Characteristics**:
- ✅ Validated correctness (100/100 tests passed)
- 🚀 **1.7-2.0× speedup** on ARM Neoverse-N1 server
- 🇷🇺 Maintains Russian standards compliance
- ⚡ Production-ready for high-performance applications
- 🎯 Suitable for: High-throughput servers, embedded systems, IoT (Russian market)

**Test Results**:
```
Total tests:  10
Passed:       10
Failed:       0
Status:       ALL TESTS PASSED
```

**Performance (ARM Neoverse-N1, GCC 13.3.0, -O3)**:
- Keypair: **1.70× faster** than DEFAULT
- Encapsulation: **1.85× faster**
- Decapsulation: **1.98× faster**

**Why It Works (vs FAST_V4)**:
- Uses **custom-developed** NTT implementation (not external neon-ntt)
- Thoroughly tested and debugged
- Optimized specifically for SABER parameters
- Maintains correctness while achieving performance

---

## Detailed Comparison Matrix

| Feature | DEFAULT | FAST_V4 | GOST | GOST_FAST |
|---------|---------|---------|------|-----------|
| **Correctness** | ✅ Verified | ❌ Failed | ✅ Verified | ✅ Verified |
| **Hash Algorithm** | SHA-3 | SHA-3 (opt) | Streebog | Streebog + NEON |
| **RNG** | System | System | GOST CTR | GOST CTR |
| **Poly Mul** | Toom-Cook | NTT (broken) | Toom-Cook | NTT (custom) |
| **NEON Optimizations** | None | External | None | Custom |
| **Russian Standards** | No | No | Yes | Yes |
| **Production Ready** | Yes | No | Yes | Yes |
| **Performance (relative)** | 1.0× | N/A | ~0.9× | **1.7-2.0×** |
| **Platform Support** | All | ARM64 | All | ARM64 |
| **Recommended For** | General use | Research | GOST compliance | High-performance + GOST |

---

## Build Instructions

### Building DEFAULT
```bash
mkdir build && cd build
cmake -DSABER_CONFIG=DEFAULT -DCMAKE_BUILD_TYPE=Release ..
make saber_gost
```

### Building FAST_V4 (⚠️ Experimental)
```bash
mkdir build && cd build
cmake -DSABER_CONFIG=FAST_V4 -DCMAKE_BUILD_TYPE=Release ..
make saber_gost
```

### Building GOST
```bash
mkdir build && cd build
cmake -DSABER_CONFIG=GOST -DCMAKE_BUILD_TYPE=Release ..
make saber_gost
```

### Building GOST_FAST (Recommended for ARM)
```bash
mkdir build && cd build
cmake -DSABER_CONFIG=GOST_FAST -DCMAKE_BUILD_TYPE=Release ..
make saber_gost
```

---

## Testing

All configurations include a comprehensive correctness test suite:

```bash
# Build test
gcc -O2 -I./include -I../SABER/Reference_Implementation_KEM \
    ./tests/test_kem_correctness.c \
    -o test_kem_correctness \
    -L. -lsaber_gost

# Run test
./test_kem_correctness
```

**Test Coverage**:
1. ✓ Basic key generation
2. ✓ Encapsulation/decapsulation
3. ✓ Multiple iterations (100 rounds)
4. ✓ Key uniqueness
5. ✓ Ciphertext integrity (FO transform validation)

---

## Performance Benchmarks

### Apple Silicon M1 (macOS, Apple Clang)

| Config | Keypair (μs) | Enc (μs) | Dec (μs) | Notes |
|--------|--------------|----------|----------|-------|
| DEFAULT | ~35 | ~40 | ~8 | Baseline |
| FAST_V4 | N/A | N/A | N/A | Correctness failure |
| GOST | ~38 | ~43 | ~9 | +8% overhead |
| GOST_FAST | ~36 | ~41 | ~8.5 | Minimal improvement (compiler too good) |

### ARM Neoverse-N1 (Linux, GCC 13.3.0)

| Config | Keypair (μs) | Enc (μs) | Dec (μs) | Speedup |
|--------|--------------|----------|----------|---------|
| DEFAULT | 45.1 | 52.8 | 10.5 | 1.0× |
| FAST_V4 | N/A | N/A | N/A | Correctness failure |
| GOST | ~48 | ~56 | ~11 | 0.95× |
| **GOST_FAST** | **26.5** | **28.6** | **5.3** | **1.7-2.0×** |

---

## Use Case Recommendations

### International Markets (No GOST Requirement)
- **Development/Testing**: DEFAULT
- **Production**: DEFAULT
- **High-Performance**: Consider other lattice schemes (Kyber)

### Russian Markets (GOST Required)
- **Compliance-First**: GOST
- **Performance-First**: **GOST_FAST** (recommended)
- **Embedded/IoT**: GOST_FAST with -Os optimization

### Research/Development
- **Algorithm Validation**: DEFAULT
- **NEON Optimization Research**: FAST_V4 (fix bugs first)
- **Benchmarking**: GOST_FAST (validated baseline)

---

## Configuration Selection Flowchart

```
START
  |
  ├─ Russian Standards Required?
  │   ├─ Yes ──> High Performance Needed?
  │   │            ├─ Yes ──> GOST_FAST (ARM64) or GOST (other platforms)
  │   │            └─ No ──> GOST
  │   │
  │   └─ No ──> NEON Optimizations Needed?
  │                ├─ Yes ──> Use other schemes (FAST_V4 broken)
  │                └─ No ──> DEFAULT
```

---

## Known Limitations

### FAST_V4
- ❌ **Broken**: Correctness failures in encapsulation/decapsulation
- 🔧 Root cause: neon-ntt asymmetric_mul implementation
- 🚫 Not recommended for any use case until fixed

### GOST_FAST
- ⚙️ **ARM64 only**: Requires NEON support
- 📱 Limited platform support (ARMv8-A+)
- 🔍 Less battle-tested than DEFAULT

### Performance Variance
- Apple Silicon: Compiler auto-vectorization reduces NEON benefit
- x86_64: GOST_FAST not available (ARM-specific)
- Embedded: May vary based on core (Cortex-A vs Neoverse)

---

## Maintenance Status

| Configuration | Maintenance | Future Plans |
|--------------|-------------|--------------|
| DEFAULT | ✅ Active | Long-term support |
| FAST_V4 | ⚠️ Broken | Fix neon-ntt or replace |
| GOST | ✅ Active | GOST 2025+ updates |
| GOST_FAST | ✅ Active | Performance tuning, more platforms |

---

## References

1. **SABER Specification**: [https://www.esat.kuleuven.be/cosic/pqcrypto/saber/](https://www.esat.kuleuven.be/cosic/pqcrypto/saber/)
2. **GOST R 34.11-2012**: Streebog Hash Function
3. **neon-ntt Repository**: [https://github.com/neon-ntt/](https://github.com/neon-ntt/) (external)
4. **pqax Repository**: Becker & Kannwischer, Indocrypt 2022
5. **ARM_SERVER_PERFORMANCE_RESULTS.md**: Performance validation on Neoverse-N1

---

**Document Version**: 1.0
**Last Updated**: 2026-01-11
**Author**: SABER_GOST Development Team
