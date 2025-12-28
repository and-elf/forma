#!/bin/bash
# Test the LSP server manually

echo "Testing Forma LSP Server..."

# Create a simple initialize request
cat <<'EOF' | ./forma_lsp_server_stdio 2>&1
Content-Length: 81

{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootUri":"file://"}}
Content-Length: 55

{"jsonrpc":"2.0","method":"initialized","params":{}}
Content-Length: 337

{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///test.forma","languageId":"forma","version":1,"text":"Foo: Column {\n    x: 10\n}\n\nBar {\n    width: nonexistent\n}\n\nBaz: Foo {\n    y: 20\n}\n"}}}
Content-Length: 43

{"jsonrpc":"2.0","id":2,"method":"shutdown"}
Content-Length: 35

{"jsonrpc":"2.0","method":"exit"}
EOF
