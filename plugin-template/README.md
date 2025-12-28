# Forma Plugin Template

Template repository for creating Forma language plugins.

## Quick Start

1. Clone this template:
   ```bash
   git clone https://github.com/forma-lang/plugin-template my-forma-plugin
   cd my-forma-plugin
   ```

2. Rename the plugin:
   - Update `CMakeLists.txt` project name
   - Update `README.md`
   - Rename source files in `src/`

3. Install Forma core library:
   ```bash
   # From Forma repository
   cd /path/to/forma
   cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
   cmake --build build --target install
   ```

4. Build your plugin:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

## Plugin Types

Forma supports several plugin types:

### Renderer Plugins

Generate code for UI frameworks (LVGL, Qt, web, etc.)

Example: `lvgl-renderer`, `qt-renderer`

### LSP/IDE Plugins

Language server features, integrations, formatters

Example: `lsp-server`, `prettier-formatter`

### Build System Plugins

Generate build files for different systems

Example: `cmake-generator`, `ninja-generator`

### Backend Plugins

Code generation for different platforms

Example: `c-backend`, `rust-backend`, `wasm-backend`

## Plugin Structure

```
my-plugin/
├── CMakeLists.txt          # CMake configuration
├── README.md               # Plugin documentation
├── src/
│   ├── my_plugin.hpp       # Public API header
│   └── my_plugin.cpp       # Implementation (if needed)
├── tests/
│   └── my_plugin_tests.cpp # Unit tests
├── examples/
│   └── example.cpp         # Usage examples
└── cmake/
    └── MyPluginConfig.cmake.in
```

## Plugin API

All plugins have access to Forma core:

```cpp
#include <forma/core/ir.hpp>      // IR structures
#include <forma/core/forma.hpp>   // Core types
#include <forma/core/plugin.hpp>  // Plugin interface

// Your plugin namespace
namespace forma::myplugin {
    // Implementation
}
```

## CMake Integration

Your plugin should provide a CMake config package:

```cmake
find_package(MyFormaPlugin REQUIRED)
target_link_libraries(app PRIVATE FormaPlugins::my_plugin)
```

## Testing

Include tests in `tests/` directory:

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## Documentation

Document your plugin:
- API reference in header comments
- Usage examples in `examples/`
- Integration guide in README.md

## Publishing

When ready to publish:

1. Create GitHub repository
2. Tag releases (v1.0.0, v1.1.0, etc.)
3. Add to Forma plugin registry (coming soon)

## Examples

See official plugins for reference:
- [lvgl-renderer](https://github.com/forma-lang/lvgl-renderer) - UI code generation
- [lsp-server](https://github.com/forma-lang/lsp-server) - Language server
- [cmake-generator](https://github.com/forma-lang/cmake-generator) - Build system

## Support

- Forma Documentation: https://forma-lang.org/docs
- Plugin Development Guide: https://forma-lang.org/docs/plugins
- Community: https://github.com/forma-lang/forma/discussions
