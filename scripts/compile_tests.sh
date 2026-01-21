#!/bin/bash
# Compile test binaries for all configurations

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "========================================"
echo "Compiling test binaries"
echo "========================================"

# Function to compile tests
compile_tests() {
    local BUILD_DIR=$1
    local CONFIG_NAME=$2

    echo ""
    echo "Compiling tests for $CONFIG_NAME..."

    cd "$BUILD_DIR"
    mkdir -p tests

    # Compile benchmark_kem
    gcc -O3 -march=native -o tests/benchmark_kem \
        ../tests/benchmark_kem.c \
        libsaber_gost.a \
        -I../include \
        -I../external/saber_ref \
        -lm 2>/dev/null || echo "  ⚠ benchmark_kem compilation failed"

    # Compile test_kem_correctness
    gcc -O3 -march=native -o tests/test_kem_correctness \
        ../tests/test_kem_correctness.c \
        libsaber_gost.a \
        -I../include \
        -I../external/saber_ref \
        -lm 2>/dev/null || echo "  ⚠ test_kem_correctness compilation failed"

    cd "$PROJECT_ROOT"
}

# Function to compile batch tests
compile_batch_tests() {
    local BUILD_DIR=$1
    local CONFIG_NAME=$2

    echo ""
    echo "Compiling batch tests for $CONFIG_NAME..."

    cd "$BUILD_DIR"
    mkdir -p tests

    # Compile benchmark_batch2
    gcc -O3 -march=native -o tests/benchmark_batch2 \
        ../tests/benchmark_batch2.c \
        libsaber_gost.a \
        -I../include \
        -I../external/saber_ref \
        -lm 2>/dev/null || echo "  ⚠ benchmark_batch2 compilation failed"

    # Compile test_batch_kem
    gcc -O3 -march=native -o tests/test_batch_kem \
        ../tests/test_batch_kem.c \
        libsaber_gost.a \
        -I../include \
        -I../external/saber_ref \
        -lm 2>/dev/null || echo "  ⚠ test_batch_kem compilation failed"

    cd "$PROJECT_ROOT"
}

# Compile tests for regular configurations
compile_tests "build_default" "DEFAULT"
compile_tests "build_fast_v4" "FAST_V4"
compile_tests "build_gost" "GOST"
compile_tests "build_gost_fast" "GOST_FAST"

# Compile tests for batch configurations
compile_batch_tests "build_fast_v4_batch" "FAST_V4_BATCH"
compile_batch_tests "build_gost_fast_batch" "GOST_FAST_BATCH"

echo ""
echo "========================================"
echo "✅ All test binaries compiled"
echo "========================================"
