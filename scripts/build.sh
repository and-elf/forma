#!/bin/bash
# Forma build script

set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "Building Forma ($BUILD_TYPE)..."

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DFORMA_BUILD_TESTS=ON \
    -DFORMA_BUILD_PLUGINS=ON

# Build
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "Binary: $BUILD_DIR/forma"
echo ""
echo "Run tests with: cd $BUILD_DIR && ./forma_test_runner"
