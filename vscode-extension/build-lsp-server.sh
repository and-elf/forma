#!/bin/bash
set -e

echo "=== Building Forma Language Server for VSCode ==="
echo ""

cd "$(dirname "$0")/.."

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure
echo "Configuring CMake..."
cmake ..

# Build just the stdio language server
echo ""
echo "Building forma_lsp_server_stdio..."
cmake --build . --target forma_lsp_server_stdio

# Check result
if [ -f "forma_lsp_server_stdio" ]; then
    echo ""
    echo "✓ Successfully built forma_lsp_server_stdio"
    echo "  Location: $(pwd)/forma_lsp_server_stdio"
    echo ""
    echo "Next: Set up the VSCode extension"
    echo "  cd ../vscode-extension"
    echo "  ./setup.sh"
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi
