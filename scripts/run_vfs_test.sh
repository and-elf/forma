#!/bin/bash
# Quick test script for VirtualFS

cd /home/andreas/work/forma

echo "Compiling VirtualFS test..."
g++ -std=c++20 -I. test_virtual_fs.cpp -o test_vfs 2>&1

if [ $? -eq 0 ]; then
    echo "Running tests..."
    ./test_vfs
else
    echo "Compilation failed"
    exit 1
fi
