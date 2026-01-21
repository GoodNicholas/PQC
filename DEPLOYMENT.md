# SABER_GOST Production Deployment Guide

This guide explains how to build and benchmark all SABER_GOST configurations on a fresh system.

## Prerequisites

- **Operating System**: Linux (ARM64 recommended) or macOS
- **Compiler**: GCC 9+ or Clang 12+
- **CMake**: 3.15 or higher
- **Build tools**: make, git (optional)

### Installing Prerequisites on Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake git
```

### Installing Prerequisites on CentOS/RHEL

```bash
sudo yum groupinstall "Development Tools"
sudo yum install -y cmake git
```

## Quick Start: Build All Configurations

This project includes a comprehensive build script that compiles all 6 configurations:

**Configurations WITHOUT batching:**
- DEFAULT (SHA3 + Toom-Cook)
- FAST_V4 (SHA3 + neon-ntt)
- GOST (Streebog + Toom-Cook)
- GOST_FAST (Streebog + Incomplete-NTT)

**Configurations WITH batching:**
- FAST_V4 + BATCHING (2x parallel operations)
- GOST_FAST + BATCHING (2x parallel operations)

### Build Command

```bash
cd SABER_GOST_PRODUCTION
./scripts/build_all_complete.sh
```

This will create 6 build directories:
- `build_default/`
- `build_fast_v4/`
- `build_gost/`
- `build_gost_fast/`
- `build_fast_v4_batch/`
- `build_gost_fast_batch/`

Build time: ~2-5 minutes depending on CPU.

## Running Benchmarks

After building, run comprehensive benchmarks:

```bash
./scripts/benchmark_all_complete.sh
```

This will:
1. Run benchmarks for all 6 configurations
2. Save results to `benchmark_results_YYYYMMDD_HHMMSS/`
3. Display a summary table
4. Save system information

Benchmark time: ~5-10 minutes for all configurations.

## Manual Build (Individual Configurations)

If you prefer to build configurations individually:

### DEFAULT Configuration

```bash
mkdir build_default && cd build_default
cmake -DSABER_CONFIG=DEFAULT ..
make -j$(nproc)
./tests/benchmark_kem
cd ..
```

### FAST_V4 Configuration (requires ARM64)

```bash
mkdir build_fast_v4 && cd build_fast_v4
cmake -DSABER_CONFIG=FAST_V4 ..
make -j$(nproc)
./tests/benchmark_kem
cd ..
```

### FAST_V4 with Batching (requires ARM64)

```bash
mkdir build_fast_v4_batch && cd build_fast_v4_batch
cmake -DSABER_CONFIG=FAST_V4 -DENABLE_BATCHING=ON ..
make -j$(nproc)
./tests/benchmark_batch2
cd ..
```

### GOST Configuration

```bash
mkdir build_gost && cd build_gost
cmake -DSABER_CONFIG=GOST ..
make -j$(nproc)
./tests/benchmark_kem
cd ..
```

### GOST_FAST Configuration (requires ARM64)

```bash
mkdir build_gost_fast && cd build_gost_fast
cmake -DSABER_CONFIG=GOST_FAST ..
make -j$(nproc)
./tests/benchmark_kem
cd ..
```

### GOST_FAST with Batching (requires ARM64)

```bash
mkdir build_gost_fast_batch && cd build_gost_fast_batch
cmake -DSABER_CONFIG=GOST_FAST -DENABLE_BATCHING=ON ..
make -j$(nproc)
./tests/benchmark_batch2
cd ..
```

## Testing Correctness

Each configuration includes correctness tests:

```bash
# For regular configurations
./build_default/tests/test_kem_correctness
./build_fast_v4/tests/test_kem_correctness
./build_gost/tests/test_kem_correctness
./build_gost_fast/tests/test_kem_correctness

# For batching configurations
./build_fast_v4_batch/tests/test_batch_kem
./build_gost_fast_batch/tests/test_batch_kem
```

All tests should pass 10/10 checks.

## Remote Deployment

### Transfer to Remote Server

```bash
# From local machine, package the directory
tar czf saber_gost_production.tar.gz SABER_GOST_PRODUCTION/

# Transfer to remote server
scp saber_gost_production.tar.gz user@server:/path/to/destination/

# On remote server
ssh user@server
cd /path/to/destination/
tar xzf saber_gost_production.tar.gz
cd SABER_GOST_PRODUCTION
```

### Build and Benchmark on Remote Server

```bash
# Build all configurations
./scripts/build_all_complete.sh

# Run all benchmarks
./scripts/benchmark_all_complete.sh

# Results will be in benchmark_results_*/
ls -la benchmark_results_*/
```

### Download Results Back to Local Machine

```bash
# From local machine
scp -r user@server:/path/to/SABER_GOST_PRODUCTION/benchmark_results_*/ ./
```

## Expected Performance

### On Apple M4 Max (ARMv9.2-A)

| Operation | DEFAULT | FAST_V4 | GOST | GOST_FAST | FAST_V4_BATCH | GOST_FAST_BATCH |
|-----------|---------|---------|------|-----------|---------------|-----------------|
| KeyGen    | ~15 μs  | ~8 μs   | ~18 μs | ~18 μs  | ~15 μs (2x)   | ~48 μs (2x)     |
| Encaps    | ~19 μs  | ~10 μs  | ~26 μs | ~26 μs  | ~19 μs (2x)   | ~63 μs (2x)     |
| Decaps    | ~23 μs  | ~11 μs  | ~29 μs | ~29 μs  | ~21 μs (2x)   | ~71 μs (2x)     |

### On ARM Server (ARMv8-A)

| Operation | DEFAULT | FAST_V4 | GOST | GOST_FAST | FAST_V4_BATCH | GOST_FAST_BATCH |
|-----------|---------|---------|------|-----------|---------------|-----------------|
| KeyGen    | ~50 μs  | ~32 μs  | ~64 μs | ~61 μs  | ~64 μs (2x)   | ~122 μs (2x)    |
| Encaps    | ~63 μs  | ~38 μs  | ~87 μs | ~80 μs  | ~77 μs (2x)   | ~161 μs (2x)    |
| Decaps    | ~73 μs  | ~44 μs  | ~108 μs| ~90 μs  | ~88 μs (2x)   | ~181 μs (2x)    |

## Troubleshooting

### Build Fails with "NEON not supported"

**Solution**: FAST_V4 and GOST_FAST require ARM64 architecture. Use DEFAULT or GOST on x86_64.

### CMake version too old

**Solution**: Install newer CMake:
```bash
# Ubuntu/Debian
sudo apt install -y software-properties-common
sudo add-apt-repository ppa:george-edison55/cmake-3.x
sudo apt update
sudo apt install cmake

# Or download from cmake.org
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-aarch64.tar.gz
tar xzf cmake-3.27.0-linux-aarch64.tar.gz
export PATH=$(pwd)/cmake-3.27.0-linux-aarch64/bin:$PATH
```

### Benchmark shows all zeros

**Solution**: Make sure you're running the binary, not just compiling:
```bash
./build_default/tests/benchmark_kem
```

## Clean Build

To start fresh:

```bash
# Remove all build directories
rm -rf build_*/

# Rebuild
./scripts/build_all_complete.sh
```

## Documentation

- [Main README](README.md) - Project overview
- [Architecture](docs/architecture/ARCHITECTURE.md) - Technical architecture
- [Quickstart](docs/guides/QUICKSTART.md) - Getting started guide
- [Full Documentation Index](docs/README.md) - All documentation

## Support

For issues or questions:
- Check documentation in `docs/`
- Review build logs in `build_*/`
- Check CMakeLists.txt for configuration options

## License

See [README.md](README.md) for license information.
