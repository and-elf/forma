#!/bin/bash

# Test the Forma LSP HTTP server

echo "Testing LSP Server on http://localhost:8080"
echo "==========================================="

# Test 1: Initialize
echo -e "\n1. Testing initialize..."
curl -X POST http://localhost:8080 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
      "processId": 1234,
      "rootUri": "file:///workspace"
    }
  }' | python3 -m json.tool 2>/dev/null || echo "Response received"

# Test 2: Open document with valid code
echo -e "\n2. Testing textDocument/didOpen with valid code..."
curl -X POST http://localhost:8080 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 2,
    "method": "textDocument/didOpen",
    "params": {
      "textDocument": {
        "uri": "file:///test.fml",
        "languageId": "forma",
        "version": 1,
        "text": "Point { property x: int property y: int }"
      }
    }
  }'

# Test 3: Get diagnostics
echo -e "\n\n3. Testing textDocument/diagnostic..."
curl -X POST http://localhost:8080 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 3,
    "method": "textDocument/diagnostic",
    "params": {
      "textDocument": {
        "uri": "file:///test.fml"
      }
    }
  }' | python3 -m json.tool 2>/dev/null || echo "Response received"

# Test 4: Open document with error
echo -e "\n\n4. Testing textDocument/didOpen with unknown type..."
curl -X POST http://localhost:8080 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 4,
    "method": "textDocument/didOpen",
    "params": {
      "textDocument": {
        "uri": "file:///error.fml",
        "languageId": "forma",
        "version": 1,
        "text": "MyRect { property pos: UnknownType }"
      }
    }
  }'

# Test 5: Get diagnostics for error document
echo -e "\n\n5. Testing textDocument/diagnostic for error document..."
curl -X POST http://localhost:8080 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 5,
    "method": "textDocument/diagnostic",
    "params": {
      "textDocument": {
        "uri": "file:///error.fml"
      }
    }
  }' | python3 -m json.tool 2>/dev/null || echo "Response received"

echo -e "\n\nAll tests completed!"
