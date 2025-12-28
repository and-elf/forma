# Building Forma

This document describes how to build the Forma programming language core library and related components.

## Requirements

- CMake 3.20 or higher
- C++20 compatible compiler:
  - GCC 10+
  - Clang 10+
  - MSVC 2019+

## Quick Start

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

## Build Options

### Core Options

- `FORMA_BUILD_TESTS` (default: ON) - Build unit tests
- `FORMA_BUILD_PLUGINS` (default: ON) - Build bundled plugins
- `FORMA_BUILD_DEMOS` (default: ON) - Build demo applications
- `FORMA_WARNINGS_AS_ERRORS` (default: OFF) - Treat compiler warnings as errors

### Sanitizer Options

Forma supports various sanitizers for detecting bugs at runtime. **Note:** Sanitizers are only available on GCC/Clang and may conflict with each other.

- `FORMA_ENABLE_SANITIZER_ADDRESS` (default: OFF) - Detect memory errors (use-after-free, buffer overflows, etc.)
- `FORMA_ENABLE_SANITIZER_LEAK` (default: OFF) - Detect memory leaks
- `FORMA_ENABLE_SANITIZER_UNDEFINED` (default: OFF) - Detect undefined behavior
- `FORMA_ENABLE_SANITIZER_THREAD` (default: OFF) - Detect data races (conflicts with address/leak sanitizers)
- `FORMA_ENABLE_SANITIZER_MEMORY` (default: OFF) - Detect uninitialized memory reads (Clang only, conflicts with other sanitizers)

## Build Type

CMake supports several build types:

- `Debug` - No optimization, full debug symbols
- `Release` - Full optimization, no debug symbols
- `RelWithDebInfo` (default) - Optimized with debug symbols
- `MinSizeRel` - Optimized for size

Set the build type:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

## Compiler Warnings

Forma uses extensive compiler warnings based on [C++ Best Practices](https://github.com/cpp-best-practices/cmake_template):

### GCC/Clang Warnings
- `-Wall -Wextra -Wpedantic` - Standard warnings
- `-Wshadow` - Variable shadowing
- `-Wnon-virtual-dtor` - Non-virtual destructors
- `-Wold-style-cast` - C-style casts
- `-Wcast-align` - Performance issues with casts
- `-Wunused` - Unused variables/functions
- `-Woverloaded-virtual` - Virtual function overloading
- `-Wconversion -Wsign-conversion` - Type conversion issues
- `-Wnull-dereference` - Null pointer dereferences
- `-Wdouble-promotion` - Float to double promotion
- `-Wformat=2` - Printf format security

### GCC-Specific Warnings
- `-Wmisleading-indentation` - Misleading indentation
- `-Wduplicated-cond` - Duplicated if/else conditions
- `-Wduplicated-branches` - Duplicated if/else branches
- `-Wlogical-op` - Logical operations that should be bitwise
- `-Wuseless-cast` - Redundant casts

### MSVC Warnings
- `/W4` - High warning level
- `/permissive-` - Standards conformance
- Various `/w1xxxx` warnings for specific issues

To treat warnings as errors (recommended for development):

```bash
cmake -B build -DFORMA_WARNINGS_AS_ERRORS=ON
```

## Example Configurations

### Development Build with Sanitizers

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DFORMA_WARNINGS_AS_ERRORS=ON \
  -DFORMA_ENABLE_SANITIZER_ADDRESS=ON \
  -DFORMA_ENABLE_SANITIZER_UNDEFINED=ON

cmake --build build
cd build && ctest
```

### Release Build

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DFORMA_BUILD_TESTS=OFF \
  -DFORMA_BUILD_DEMOS=OFF

cmake --build build
sudo cmake --install build
```

### Testing for Thread Safety

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DFORMA_ENABLE_SANITIZER_THREAD=ON

cmake --build build
cd build && ctest
```

## Colored Compiler Output

Forma automatically enables colored compiler diagnostics:
- GCC: `-fdiagnostics-color=always`
- Clang: `-fcolor-diagnostics`  
- MSVC: `/diagnostics:column`

## Installation

```bash
# Install to default prefix (/usr/local)
sudo cmake --install build

# Install to custom prefix
cmake --install build --prefix /opt/forma
```

This installs:
- Header files to `include/forma/`
- CMake package files to `lib/cmake/FormaCore/`
- Documentation to `share/doc/forma/`

## Using Forma in Your Project

After installation, you can use Forma in your CMake project:

```cmake
find_package(FormaCore REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE FormaCore::forma_core)
```

## Troubleshooting

### Sanitizer Conflicts

Sanitizers have incompatibilities:
- Thread sanitizer conflicts with Address/Leak sanitizers
- Memory sanitizer (Clang) conflicts with all other sanitizers
- Enable only one at a time for best results

### Warning Overload

If you get too many warnings from dependencies:
- Set `FORMA_WARNINGS_AS_ERRORS=OFF` during initial development
- Use `target_compile_options` to disable specific warnings for problematic headers

### Build Performance

For faster builds:
- Use Ninja: `cmake -B build -G Ninja`
- Enable ccache (if available): `export CMAKE_CXX_COMPILER_LAUNCHER=ccache`
- Reduce test builds: `-DFORMA_BUILD_TESTS=OFF`

## Additional Resources

- [CMake Best Practices Template](https://github.com/cpp-best-practices/cmake_template)
- [Forma Documentation](README.md)
- [Plugin Development](plugins/README.md)
