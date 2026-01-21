#!/bin/bash
# Build all SABER_GOST configurations (with and without batching)

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "========================================="
echo "Building ALL SABER_GOST configurations"
echo "Including batching variants"
echo "========================================="

# Configurations without batching
CONFIGS_NO_BATCH=("DEFAULT" "FAST_V4" "GOST" "GOST_FAST")

# Configurations with batching support
CONFIGS_WITH_BATCH=("FAST_V4" "GOST_FAST")

echo ""
echo "Phase 1: Building configurations WITHOUT batching"
echo "================================================="

for CONFIG in "${CONFIGS_NO_BATCH[@]}"; do
    echo ""
    echo "Building $CONFIG configuration..."

    BUILD_DIR="build_$(echo $CONFIG | tr '[:upper:]' '[:lower:]')"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake -DSABER_CONFIG="$CONFIG" ..
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    echo "✅ $CONFIG build complete"
    cd "$PROJECT_ROOT"
done

echo ""
echo "Phase 2: Building configurations WITH batching"
echo "=============================================="

for CONFIG in "${CONFIGS_WITH_BATCH[@]}"; do
    echo ""
    echo "Building $CONFIG + BATCHING configuration..."

    BUILD_DIR="build_$(echo $CONFIG | tr '[:upper:]' '[:lower:]')_batch"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake -DSABER_CONFIG="$CONFIG" -DENABLE_BATCHING=ON ..
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    echo "✅ $CONFIG + BATCHING build complete"
    cd "$PROJECT_ROOT"
done

echo ""
echo "========================================="
echo "All configurations built successfully!"
echo "========================================="
echo ""
echo "Build directories created:"
echo ""
echo "Without batching:"
for CONFIG in "${CONFIGS_NO_BATCH[@]}"; do
    echo "  - build_$(echo $CONFIG | tr '[:upper:]' '[:lower:]')"
done
echo ""
echo "With batching:"
for CONFIG in "${CONFIGS_WITH_BATCH[@]}"; do
    echo "  - build_$(echo $CONFIG | tr '[:upper:]' '[:lower:]')_batch"
done
echo ""
echo "Total: 6 configurations built"
echo ""
