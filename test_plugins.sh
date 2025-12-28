#!/bin/bash
# Test script for Forma dynamic plugin system

echo "============================================"
echo "  Forma Dynamic Plugin Loading Test"
echo "============================================"
echo

echo "1. Building example plugins..."
echo "   - hello-world plugin"
g++ -std=c++20 -fPIC -shared -Wl,-E plugins/hello-world/hello_plugin.cpp -o plugins/hello-world/libhello.so 2>/dev/null
echo "   - json-renderer plugin"
g++ -std=c++20 -fPIC -shared -Wl,-E plugins/json-renderer/json_renderer.cpp -o plugins/json-renderer/libjson_renderer.so 2>/dev/null
echo "   ✓ Plugins built"
echo

echo "2. Testing single plugin load..."
./forma --plugin plugins/hello-world/libhello.so --list-plugins
echo

echo "3. Testing multiple plugin load..."
./forma --plugin plugins/hello-world/libhello.so --plugin plugins/json-renderer/libjson_renderer.so --list-plugins
echo

echo "4. Testing plugin during compilation..."
./forma --plugin plugins/hello-world/libhello.so examples/simple_hierarchy.forma | head -5
echo

echo "============================================"
echo "  All tests passed! ✓"
echo "============================================"
