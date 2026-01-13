#!/bin/bash
# Build all SABER_GOST configurations

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "========================================="
echo "Building all SABER_GOST configurations"
echo "========================================="

CONFIGS=("DEFAULT" "FAST_V4" "GOST" "GOST_FAST")

for CONFIG in "${CONFIGS[@]}"; do
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
echo "========================================="
echo "All configurations built successfully!"
echo "========================================="
echo ""
echo "Build directories:"
for CONFIG in "${CONFIGS[@]}"; do
    echo "  - build_$(echo $CONFIG | tr '[:upper:]' '[:lower:]')"
done
