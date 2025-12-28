# LSP Server Plugin for Forma

Language Server Protocol implementation for Forma language.

## Features

- Full LSP protocol support
- Symbol resolution and navigation
- Diagnostics (errors, warnings)
- Code completion
- Virtual filesystem for in-memory editing
- HTTP server for web-based IDEs

## Building

```bash
mkdir build && cd build
cmake .. -DFormaCore_DIR=/path/to/forma/install/lib/cmake/FormaCore
cmake --build .
```

## Running

```bash
./forma_lsp_server
```

The server communicates via stdin/stdout using JSON-RPC 2.0.

## Integration

### As a CMake Dependency

```cmake
find_package(FormaLSPServer REQUIRED)
target_link_libraries(your_target PRIVATE FormaPlugins::forma_lsp)
```

### VS Code Extension

Configure in `.vscode/settings.json`:

```json
{
    "forma.languageServer.command": "forma_lsp_server"
}
```

## Documentation

- [LSP_SERVER.md](../../LSP_SERVER.md) - LSP protocol implementation
- [VIRTUAL_FS.md](../../VIRTUAL_FS.md) - Virtual filesystem documentation

## Development

Self-contained plugin. Can be moved to separate repository.
Dependencies: Forma core library only.
