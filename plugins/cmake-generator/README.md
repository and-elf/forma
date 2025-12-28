# CMake Generator Plugin for Forma

Build system plugin that generates CMakeLists.txt from Forma build definitions.

## Features

- Generate CMakeLists.txt from Forma build configurations
- Support for multiple targets
- Library dependencies
- Compile options and flags
- Plugin-based build system architecture

## Building

```bash
mkdir build && cd build
cmake .. -DFormaCore_DIR=/path/to/forma/install/lib/cmake/FormaCore
cmake --build .
```

## Usage

The plugin is loaded dynamically by the Forma build system:

```bash
forma build --generator cmake
```

## Integration

### As a CMake Dependency

```cmake
find_package(FormaCMakeGenerator REQUIRED)
target_link_libraries(your_target PRIVATE FormaPlugins::forma_cmake_generator)
```

### Build Configuration

In `forma.toml`:

```toml
[build]
generator = "cmake"
cmake_minimum_version = "3.20"
```

## Development

Self-contained plugin implementing the Forma build plugin interface.
Can be moved to separate repository.
