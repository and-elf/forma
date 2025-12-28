# Forma Plugin Architecture

This directory contains the official Forma plugins. Each plugin is designed to be self-contained and can be moved to its own repository.

## Directory Structure

```
plugins/
├── lvgl-renderer/      # LVGL C99 code generator
├── lsp-server/         # Language Server Protocol implementation  
├── cmake-generator/    # CMake build file generator
├── tracer/             # Logging and diagnostics plugin
└── README.md           # This file
```

## Building Plugins

### Build All Plugins (with Core)

From the main Forma repository:

```bash
cmake -B build -DFORMA_BUILD_PLUGINS=ON
cmake --build build
```

### Build Individual Plugin

Each plugin can be built independently if Forma core is installed:

```bash
# Install Forma core first
cd /path/to/forma
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target install

# Build a specific plugin
cd plugins/lvgl-renderer
mkdir build && cd build
cmake .. -DFormaCore_DIR=/usr/local/lib/cmake/FormaCore
cmake --build .
cmake --build . --target test  # Run tests
```

### Build Without Core Installation

If you want to build plugins without installing core (development):

```bash
# From forma root
cmake -B build
cmake --build build

# Build specific plugin referencing build tree
cd plugins/lvgl-renderer
mkdir build && cd build
cmake .. -DFormaCore_DIR=../../build
cmake --build .
```

## Plugin Overview

### LVGL Renderer

**Purpose**: Generate C99 LVGL code from Forma UI definitions

**Key Features**:
- Constexpr code generation
- 19 widget type mappings
- Complete C99 output
- Header-only library

**Usage**:
```cpp
#include <forma/plugins/lvgl-renderer/lvgl_renderer.hpp>

forma::lvgl::LVGLRenderer<16384> renderer;
renderer.generate(document);
std::cout << renderer.c_str();
```

**Docs**: [README](lvgl-renderer/README.md) | [API](../LVGL_RENDERER.md)

### LSP Server

**Purpose**: Language server for Forma with IDE integration

**Key Features**:
- Full LSP protocol support
- Symbol resolution
- Diagnostics
- Virtual filesystem
- HTTP server for web IDEs

**Usage**:
```bash
./forma_lsp_server
```

**Docs**: [README](lsp-server/README.md) | [LSP](../LSP_SERVER.md) | [VFS](../VIRTUAL_FS.md)

### Tracer Plugin

**Purpose**: Logging and diagnostics for Forma compiler pipeline

**Key Features**:
- Multiple verbosity levels (Silent, Normal, Verbose, Debug)
- Structured output with indentation
- Stage tracking with begin/end
- Error and warning highlighting
- Statistics reporting

**Usage**:
```cpp
#include "plugins/tracer/src/tracer_plugin.hpp"

using namespace forma::tracer;

auto& tracer = get_tracer();
tracer.set_level(TraceLevel::Verbose);

tracer.begin_stage("Parsing");
tracer.stat("Instances", 42);
tracer.end_stage();
```

**Docs**: [README](tracer/README.md)

### CMake Generator

**Purpose**: Generate CMakeLists.txt from Forma build configurations

**Key Features**:
- CMake generation from `.toml` configs
- Plugin-based build architecture
- Multiple target support

**Usage**: Used internally by Forma build system

**Docs**: [README](cmake-generator/README.md)

## Creating New Plugins

Use the plugin template as starting point:

```bash
# Copy template
cp -r ../plugin-template my-plugin
cd my-plugin

# Customize
# 1. Edit CMakeLists.txt (project name)
# 2. Rename files in src/
# 3. Implement your plugin logic

# Build
mkdir build && cd build
cmake .. -DFormaCore_DIR=/path/to/forma/lib/cmake/FormaCore
cmake --build .
```

See [Plugin Template README](../plugin-template/README.md) for details.

## Moving Plugins to Separate Repos

Each plugin is designed to be self-contained:

1. **Dependencies**: Only on Forma core library
2. **CMake**: Complete CMake configuration included
3. **Tests**: Self-contained test suite
4. **Docs**: Plugin-specific documentation

### Migration Steps

```bash
# 1. Create new repo
mkdir forma-lvgl-renderer
cd forma-lvgl-renderer
git init

# 2. Copy plugin
cp -r /path/to/forma/plugins/lvgl-renderer/* .

# 3. Add plugin-specific README
# Edit README.md with repo-specific info

# 4. Update CMake paths if needed
# Most should work as-is

# 5. Push to GitHub
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/forma-lang/forma-lvgl-renderer
git push -u origin main
```

## Plugin Interface

All plugins have access to:

```cpp
#include <forma/core/ir.hpp>      // IR structures (Document, TypeDecl, etc.)
#include <forma/core/forma.hpp>   // Core types and limits
#include <forma/core/plugin.hpp>  // Plugin interface definitions
#include <forma/core/toml.hpp>    // TOML parser
```

### Core IR Structures

- `Document<>` - Parsed Forma document
- `TypeDecl` - Type declarations
- `EnumDecl` - Enum definitions
- `InstanceDecl` - Widget instances
- `Parser` - Parsing utilities
- `Lexer` - Tokenization

## Testing

Each plugin includes its own test suite:

```bash
cd plugins/<plugin-name>
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DFormaCore_DIR=/path/to/core
cmake --build .
ctest  # Run tests
```

## Installation

Plugins can be installed alongside or separately from core:

```bash
# Install all
cd /path/to/forma
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target install

# Install single plugin
cd plugins/lvgl-renderer
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . --target install
```

Installed locations:
- Libraries: `/usr/local/lib/forma/plugins/`
- Headers: `/usr/local/include/forma/plugins/<plugin>/`
- CMake: `/usr/local/lib/cmake/Forma<Plugin>/`

## Integration

### In CMake Projects

```cmake
find_package(FormaLVGLRenderer REQUIRED)
target_link_libraries(myapp PRIVATE FormaPlugins::forma_lvgl_renderer)
```

### Dynamic Loading

Plugins can also be loaded dynamically at runtime (future feature).

## Contributing

To contribute a new plugin:

1. Start with plugin-template
2. Develop and test locally
3. Submit PR to add to bundled plugins
4. Once stable, can graduate to separate repo

## Support

- **Core Library**: [Forma Issues](https://github.com/forma-lang/forma/issues)
- **Plugin Issues**: File in respective plugin repos (once migrated)
- **Plugin Development**: [Plugin Template](../plugin-template/README.md)
