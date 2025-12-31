# Forma Programming Language

A QML-inspired declarative programming language with compile-time evaluation and plugin-based architecture.

## Overview

Forma is designed as a **core library** providing tokenization, parsing, and IR, with **self-contained plugins** for code generation, IDE integration, and build systems.

```
┌─────────────────────────────────────────┐
│         Forma Core Library              │
│  (Tokenizer, Parser, IR, Plugin API)    │
└────────────┬────────────────────────────┘
             │
    ┌────────┴────────┬─────────────┬────────────┐
    │                 │             │            │
┌───▼────┐      ┌────▼─────┐  ┌───▼────┐  ┌────▼─────┐
│ LVGL   │      │   LSP    │  │ CMake  │  │  Your    │
│Renderer│      │ Server   │  │  Gen   │  │ Plugin   │
└────────┘      └──────────┘  └────────┘  └──────────┘
```

## Architecture

### Core Library (Forma Core)

**Location**: `src/`

The core provides:
- **Tokenizer/Lexer**: Constexpr tokenization at compile-time
- **Parser**: Parse Forma syntax into IR structures
- **IR (Intermediate Representation)**: Type-safe document structure
- **Plugin Interface**: Standard API for extensions

**Headers**:
- `forma.hpp` - Core types, limits, configuration
- `ir.hpp` - Tokenizer, parser, and IR structures
- `plugin.hpp` - Plugin interface definitions
- `toml.hpp` - Configuration file parser
- `core/toolchain.hpp` - Cross-compilation toolchain management

### Plugin Architecture

**Location**: `plugins/`

Each plugin is a self-contained CMake project that:
- Depends only on Forma Core
- Can be built independently
- Can be moved to separate repositories
- Implements specific functionality

**Official Plugins**:

| Plugin | Purpose | Output |
|--------|---------|--------|
| **lvgl-renderer** | UI code generation | C99 LVGL code |
| **lsp-server** | IDE integration | LSP protocol (stdi
| **esp32-lvgl** | ESP32 build system | ESP-IDF projects |
| **deb-deploy** | Debian packaging | .deb packages |o/HTTP) |
| **cmake-generator** | Build system | CMakeLists.txt |

**IDE Support**:

| Tool | Location | Features |
|------|----------|----------|
| **VSCode Extension** | `vscode-extension/` | Syntax highlighting, diagnostics, completion |

See [plugins/README.md](plugins/README.md) for plugin details.

### Plugin Template

**Location**: `plugin-template/`

Starting point for creating new plugins. Copy this template to create:
- Renderer plugins (Qt, web, native)
- Backend plugins (Rust, JavaScript, WebAssembly)
- IDE plugins (formatters, linters)
- Build plugins (Ninja, Meson, Bazel)

## Building

### Install Core Library

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --build build --target install
```

This installs:
- Headers: `/usr/local/include/forma/core/`
- CMake config: `/usr/local/lib/cmake/FormaCore/`

### Build with Bundled Plugins

```bash
cmake -B build -DFORMA_BUILD_PLUGINS=ON
cmake --build build
sudo cmake --build build --target install
```

### Build Individual Plugin

After installing core:

```bash
cd plugins/lvgl-renderer
cmake -B build -DFormaCore_DIR=/usr/local/lib/cmake/FormaCore
cmake --build build
cmake --build build --target test
```

## Usage

### In CMake Projects

**Core only**:
```cmake
find_package(FormaCore REQUIRED)
target_link_libraries(myapp PRIVATE FormaCore::forma_core)
```

**With plugins**:
```cmake
find_package(FormaCore REQUIRED)
find_package(FormaLVGLRenderer REQUIRED)

target_link_libraries(myapp PRIVATE 
    FormaCore::forma_core
    FormaPlugins::forma_lvgl_renderer
)
```

### Parsing Forma Code

```cpp
#include <forma/core/ir.hpp>

constexpr const char* source = R"(
    enum Status { Active, Inactive }
    
    Button {
        text: "Click Me"
        width: 100
        height: 50
    }
)";

// Parse at compile-time
Parser parser(source);
Document<8, 8, 8, 16, 16> doc;

// Parse declarations
while (!parser.check(TokenKind::EndOfFile)) {
    if (parser.current.kind == TokenKind::Enum) {
        doc.enums[doc.enum_count++] = parse_enum(parser);
    }
    // ... handle other declarations
}
```

### Using Plugins

**LVGL Renderer**:
```cpp
#include <forma/plugins/lvgl-renderer/lvgl_renderer.hpp>

forma::lvgl::LVGLRenderer<16384> renderer;
renderer.generate(doc);

// Output C99 LVGL code
std::cout << renderer.c_str();
```

**LSP Server**:
```bash
# Run language server (stdio mode for VSCode)
./forma_lsp_server_stdio

# Or HTTP mode for web IDEs
./forma_lsp_server 8080
```

**VSCode Extension**:
```bash
# Build the language server
cmake --build build --target forma_lsp_server_stdio

# Install the extension
cd vscode-extension
./setup.sh

# Or package for distribution
npm run package
code --install-extension forma-language-*.vsix
```

See [vscode-extension/README.md](vscode-extension/README.md) for details.

## Language Features

### Type System

```forma
enum Color {
    Red,
    Green,
    Blue
}

type Button {
    property text: string
    property width: int
    property height: int
}
```

### Declarative UI

```forma
Panel {
    width: 320
    height: 240
    
    Label {
        text: "Welcome"
        x: 10
        y: 10
    }
    
    Button {
        text: "Click Me"
        x: 10
        y: 50
    }
}
```

### Properties

```forma
Button {
    text: "Submit"
    width: 100
    height: 40
    enabled: true
}
```

### Events (planned)

```forma
Button {
    text: "Click"
    
    on_click: {
        count = count + 1
    }
}
```

## Plugin Development

Create custom plugins using the template:

```bash
# Copy template
cp -r plugin-template my-plugin
cd my-plugin

# Customize
# 1. Edit CMakeLists.txt (project name, description)
# 2. Implement plugin in src/
# 3. Add tests in tests/

# Build
mkdir build && cd build
cmake .. -DFormaCore_DIR=/path/to/core
cmake --build .
ctest  # Run tests
```

### Plugin Types

- **Renderers**: Generate code for UI frameworks
  - LVGL (embedded), Qt (desktop), React (web), SwiftUI (iOS)
  
- **Backends**: Generate code in different languages
  - C, C++, Rust, JavaScript, TypeScript, Kotlin
  
- **IDE Integration**: Editor tooling
  - LSP servers, formatters, syntax highlighters
  
- **Build Systems**: Generate build configurations
  - CMake, Ninja, Meson, Bazel, Make

See [plugin-template/README.md](plugin-template/README.md) for the complete guide.

## Project Structure

```
forma/
├── CMakeLists.txt              # Core library build
├── README.md                   # This file
├── src/                        # Core library (header-only)
│   ├── forma.hpp              # Types, limits, configuration
│   ├── ir.hpp                 # Tokenizer, parser, IR
│   ├── plugin.hpp             # Plugin interface
│   ├── toml.hpp               # TOML parser
│   └── export.hpp             # C++ export utilities
├── cmake/                      # CMake configuration
│   └── FormaCoreConfig.cmake.in
├── plugins/                    # Official plugins
│   ├── README.md              # Plugin architecture guide
│   ├── lvgl-renderer/         # LVGL code generator
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   ├── src/
│   │   └── tests/
│   ├── lsp-server/            # Language server
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   ├── src/
│   │   └── tests/
│   └── cmake-generator/       # Build generator
│       ├── CMakeLists.txt
│       ├── README.md
│       └── src/
├── plugin-template/            # Template for new plugins
│   ├── README.md              # Plugin development guide
│   ├── CMakeLists.txt
│   ├── src/
│   │   └── example_plugin.hpp
│   ├── tests/
│   └── examples/
├── tests/                      # Core library tests
├── examples/                   # Example .fml files
└── docs/                       # Documentation
```

## Documentation

- **Core Library**: [src/ir.hpp](src/ir.hpp) - Inline documentation
- **LVGL Plugin**: [plugins/lvgl-renderer/LVGL_RENDERER.md](plugins/lvgl-renderer/LVGL_RENDERER.md)
- **LSP Server**: [plugins/lsp-server/LSP_SERVER.md](plugins/lsp-server/LSP_SERVER.md)
- **Virtual FS**: [plugins/lsp-server/VIRTUAL_FS.md](plugins/lsp-server/VIRTUAL_FS.md)
- **Plugin Development**: [plugin-template/README.md](plugin-template/README.md)
- **Plugin Architecture**: [plugins/README.md](plugins/README.md)

## Features

### Core Library
- ✅ 100% constexpr - parser runs at compile-time
- ✅ Header-only - no build dependencies
- ✅ Type-safe IR
- ✅ Complete error recovery
- ✅ Zero runtime dependencies
- ✅ Plugin interface

### Plugins
- ✅ LVGL renderer (C99 generation)
- ✅ LSP server (IDE integration)
- ✅ CMake generator (build system)
- ✅ Virtual filesystem
- ⏳ More renderers (Qt, web, native)
- ⏳ More backends (Rust, JS, WASM)

## Examples

See [examples/](examples/) directory for sample `.fml` files.

**Simple Button**:
```forma
Button {
    text: "Click Me"
    width: 120
    height: 40
}
```

**Dashboard Layout**:
```forma
Panel {
    width: 480
    height: 320
    
    Label {
        text: "Dashboard"
        x: 10
        y: 10
    }
    
    Slider {
        value: 50
        min: 0
        max: 100
        x: 10
        y: 50
    }
}
```

## Configuration

Use `forma.toml` or `project.toml` to configure your project:

```toml
[project]
name = "myapp"
version = "1.0.0"
renderer = "lvgl"
build_system = "cmake"

[build]
target = "esp32s3"

[esp32]
idf_version = "v5.1"
auto_install = true

[deploy]
# Deploy packaging system: deb, rpm, etc.
system = "deb"
```

### Target Configuration

Forma supports multiple build targets with automatic toolchain management:

**Native Targets**:
- `x86_64-linux-gnu` - Native x86_64 Linux
- `aarch64-linux-gnu` - ARM 64-bit Linux
- `arm-linux-gnueabihf` - ARM 32-bit Linux
- `x86_64-w64-mingw32` - Windows cross-compile
- `riscv64-linux-gnu` - RISC-V 64-bit

**Embedded Targets**:
- `stm32` / `arm-none-eabi` - STM32 ARM Cortex-M (all series)
- `esp32` - ESP32 (Xtensa LX6, WiFi + BT)
- `esp32s2` - ESP32-S2 (Xtensa LX7, WiFi + USB)
- `esp32s3` - ESP32-S3 (Xtensa LX7, WiFi + BLE + AI)
- `esp32c3` - ESP32-C3 (RISC-V, WiFi + BLE)
- `esp32c6` - ESP32-C6 (RISC-V, WiFi 6 + Zigbee)

### Automatic Toolchain Management

Forma automatically downloads and manages cross-compilation toolchains. When you specify a target, the toolchain is:
1. **Checked** on system PATH first
2. **Downloaded** to `~/.forma/toolchains/` if not found
3. **Extracted** and made available for build

**Manual toolchain check**:
```cpp
#include <forma/core/toolchain.hpp>

// Get supported targets
auto targets = forma::toolchain::ToolchainManager::get_supported_targets();

// Ensure compiler is available (downloads if needed)
std::string compiler = forma::toolchain::ToolchainManager::ensure_compiler_available("esp32s3");

// Get toolchain info
auto info = forma::toolchain::ToolchainManager::get_toolchain_info("stm32");
std::cout << "Toolchain: " << info.description << "\n";
std::cout << "Compiler: " << info.compiler_name << "\n";
```

## CLI Commands

### Initialize a New Project

```bash
forma init --name myapp --renderer lvgl --build cmake
```

Creates a new Forma project with:
- `project.toml` - Project configuration
- `src/main.fml` - Main application file
- CMakeLists.txt or other build files
- Basic directory structure

**Embedded Targets**:

```bash
# Initialize ESP32 project
forma init --target esp32 --name esp32-project

# Initialize ESP32-S3 project with LVGL
forma init --target esp32s3 --name esp32s3-project

# Initialize STM32 project
forma init --target stm32 --name stm32-project
```

This automatically:
- Configures the appropriate toolchain (xtensa, arm-none-eabi)
- Sets up target-specific build system (ESP-IDF, CMake)
- Downloads required toolchains if not available
- Creates `project.toml` with target configuration

### Build Your Project

```bash
forma build
```

Automatically detects the target from `project.toml` and builds accordingly:
- **ESP32**: Uses ESP-IDF build system (`idf.py build`)
- **Native**: Uses CMake build system
- **STM32**: Uses CMake with ARM Cortex-M toolchain

**ESP32-specific options**:
```bash
# Build and flash to device
forma build --flash

# Build, flash, and monitor serial output
forma build --flash --monitor
```

### Compile Forma Code

```bash
forma --renderer lvgl myapp.fml
```

Compiles your Forma code to the target output (C, C++, etc.)

### Release/Package Your Application

```bash
forma release
```

Reads the `[deploy]` section from `project.toml` and automatically builds a release package.

**With explicit release system**:
```bash
forma release --release-system deb
```

**Requirements**:
- `project.toml` with `[deploy]` section
- `package.cfg` with package metadata (for Debian)
- deb-deploy plugin built and available

**Example package.cfg**:
```ini
name=myapp
version=1.0.0
architecture=amd64
maintainer=Your Name <you@example.com>
description=My awesome Forma application
section=utils
priority=optional
depends=libc6,libgcc1

[postinst]
commands=systemctl enable myapp.service

[prerm]
commands=systemctl stop myapp.service
```

See `plugins/deb-deploy/README.md` for full packaging documentation.

## Build Configuration

Use `forma.toml` to configure build targets:

```toml
[build]
generator = "cmake"
target = "embedded"

[target.embedded]
renderer = "lvgl"
output = "build/ui.c"

[target.desktop]
renderer = "qt"
output = "build/ui.cpp"
```

## Testing

```bash
# Core library tests
cmake -B build -DFORMA_BUILD_TESTS=ON
cmake --build build
cmake --build build --target check

# Plugin tests
cd plugins/lvgl-renderer
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Test Coverage

Generate code coverage reports using gcovr:

```bash
# Install gcovr
pip install gcovr

# Build with coverage enabled
cmake -B build -DFORMA_BUILD_TESTS=ON -DFORMA_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests and generate coverage report
cmake --build build --target coverage

# View HTML report
xdg-open build/coverage.html  # Linux
open build/coverage.html      # macOS
```

This generates:
- `build/coverage.html` - Detailed HTML report with line-by-line coverage
- `build/coverage.txt` - Text summary of coverage statistics

## Contributing

1. **Core Library**: Submit PRs to main repository
2. **Bundled Plugins**: Submit PRs to respective plugin directories
3. **New Plugins**: Create from template, develop, submit PR

## Migration Path

Plugins can graduate to separate repositories:

1. Plugin stabilizes in `plugins/`
2. Create new repo (e.g., `forma-lang/forma-lvgl-renderer`)
3. Copy plugin directory
4. Update README and CI
5. Publish to package managers
6. Keep as bundled option or remove from main repo

## License

[License TBD]

## Links

- **Documentation**: https://forma-lang.org/docs
- **GitHub**: https://github.com/forma-lang/forma
- **Community**: https://github.com/forma-lang/forma/discussions
- **Plugin Registry**: https://forma-lang.org/plugins (coming soon)
