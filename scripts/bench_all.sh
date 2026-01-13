#!/bin/bash
# Benchmark all SABER_GOST configurations

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "========================================="
echo "Benchmarking all SABER_GOST configurations"
echo "========================================="

CONFIGS=("DEFAULT" "FAST_V4" "GOST" "GOST_FAST")

for CONFIG in "${CONFIGS[@]}"; do
    echo ""
    echo "Benchmarking $CONFIG configuration..."

    BUILD_DIR="build_$(echo $CONFIG | tr '[:upper:]' '[:lower:]')"

    if [ ! -d "$BUILD_DIR" ]; then
        echo "❌ Build directory not found: $BUILD_DIR"
        echo "   Run ./build_all.sh first"
        continue
    fi

    if [ ! -f "$BUILD_DIR/tests/benchmark_kem" ]; then
        echo "⚠️  Benchmark executable not found in $BUILD_DIR/tests/"
        continue
    fi

    cd "$BUILD_DIR/tests"
    echo "---"
    ./benchmark_kem
    echo "---"

    cd "$PROJECT_ROOT"
done

echo ""
echo "========================================="
echo "Benchmarking complete!"
echo "========================================="
