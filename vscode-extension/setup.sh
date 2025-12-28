#!/bin/bash
set -e

echo "=== Forma VSCode Extension Setup ==="
echo ""

# Check if we're in the right directory
if [ ! -f "package.json" ]; then
    echo "Error: Must run from vscode-extension directory"
    exit 1
fi

# Check for Node.js
if ! command -v node &> /dev/null; then
    echo "Error: Node.js is not installed"
    echo "Please install Node.js 18+ from https://nodejs.org/"
    exit 1
fi

echo "Node.js version: $(node --version)"
echo "npm version: $(npm --version)"
echo ""

# Install dependencies
echo "Installing dependencies..."
npm install

# Compile TypeScript
echo ""
echo "Compiling TypeScript..."
npm run compile

# Check if language server exists
LSP_SERVER="../build/forma_lsp_server_stdio"
if [ ! -f "$LSP_SERVER" ]; then
    echo ""
    echo "Warning: Language server not found at $LSP_SERVER"
    echo "Please build it first:"
    echo "  cd .."
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  cmake --build . --target forma_lsp_server_stdio"
    echo ""
else
    echo ""
    echo "✓ Language server found at $LSP_SERVER"
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  1. Open this folder in VSCode: code ."
echo "  2. Press F5 to launch Extension Development Host"
echo "  3. Open a .forma file to test"
echo ""
echo "Or package for installation:"
echo "  npm run package"
echo "  code --install-extension forma-language-*.vsix"
echo ""
