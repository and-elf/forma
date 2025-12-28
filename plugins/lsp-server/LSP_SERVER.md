# Forma LSP Server

A constexpr Language Server Protocol implementation for the Forma language with HTTP transport.

## Features

- **Full LSP Protocol**: Initialize, textDocument/didOpen, didChange, didClose, diagnostics
- **Real-time Analysis**: Two-pass semantic analysis with forward reference support
- **Type Checking**: Validates type references, generic types (Forma.Array), enums, and events
- **Constexpr Everything**: The entire LSP core is constexpr-testable at compile time
- **HTTP Transport**: Simple HTTP server for easy integration

## Building

```bash
# Build the LSP server
g++ -std=c++20 -I. lsp_server.cpp -o lsp_server

# Build the test client
g++ -std=c++20 test_lsp_client.cpp -o test_client
```

## Running

```bash
# Start the server (default port 8080)
./lsp_server

# Or specify a custom port
./lsp_server 9000
```

## Testing

The server accepts JSON-RPC 2.0 messages via HTTP POST.

### Example: Initialize

```bash
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
  }'
```

Response:
```json
{
  "jsonrpc": 2,
  "id": 1,
  "result": {
    "capabilities": {
      "textDocumentSync": {
        "openClose": true,
        "change": 1
      },
      "diagnosticProvider": true
    },
    "serverInfo": {
      "name": "forma-lsp",
      "version": "0.1.0"
    }
  }
}
```

### Example: Open Document

```bash
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
```

### Example: Get Diagnostics

```bash
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
  }'
```

Response (no errors):
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "kind": "full",
    "items": []
  }
}
```

### Example: Document with Errors

```bash
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
```

Then get diagnostics:
```bash
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
  }'
```

Response (with error):
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "result": {
    "kind": "full",
    "items": [
      {
        "range": {
          "start": {"line": 0, "character": 0},
          "end": {"line": 0, "character": 11}
        },
        "severity": 1,
        "code": "unknown-type",
        "message": "UnknownType"
      }
    ]
  }
}
```

## Architecture

### Core Components

1. **lsp.hpp**: LSP protocol types and document manager (constexpr)
2. **http_server.hpp**: HTTP transport layer with JSON handling
3. **lsp_server.cpp**: Main server executable

### Symbol Resolution

The LSP uses a two-pass analysis approach:

**Pass 1**: Parse all declarations and build symbol table
- Collect all type, enum, and event declarations
- Add symbols to the table

**Pass 2**: Validate type references
- Check all property types against symbol table
- Check event parameter types
- Generate diagnostics for unknown types

This allows forward references - types can be used before they're defined in the file.

### Diagnostic Severities

- **1 (Error)**: Type errors, unknown types, invalid syntax
- **2 (Warning)**: Non-critical issues
- **3 (Information)**: Informational messages
- **4 (Hint)**: Code improvement suggestions

## Supported LSP Methods

- `initialize`: Server initialization with capabilities
- `textDocument/didOpen`: Open and analyze a document
- `textDocument/didChange`: Update document content (full sync)
- `textDocument/didClose`: Close a document
- `textDocument/diagnostic`: Get diagnostics for a document

## Implementation Notes

- Uses C++20 constexpr for compile-time testing
- No external dependencies (except standard library)
- Single-threaded synchronous request handling
- Full document synchronization (change=1)
- Supports up to 16 concurrent documents
- Up to 32 diagnostics per document
- Symbol table capacity: 128 symbols
