# LVGL Renderer Plugin for Forma

This plugin generates C99 LVGL code from Forma UI definitions.

## Features

- Constexpr code generation
- 19 widget type mappings (Button, Label, Slider, etc.)
- 11+ property translations
- Complete C99 output with LVGL API calls
- Enum generation support

## Building

This plugin requires the Forma core library.

```bash
mkdir build && cd build
cmake .. -DFormaCore_DIR=/path/to/forma/install/lib/cmake/FormaCore
cmake --build .
```

## Integration

### As a CMake Dependency

```cmake
find_package(FormaLVGLRenderer REQUIRED)
target_link_libraries(your_target PRIVATE FormaPlugins::forma_lvgl_renderer)
```

### Standalone Usage

```cpp
#include <forma/plugins/lvgl-renderer/lvgl_renderer.hpp>

using namespace forma::lvgl;

// Create a document
Document<4, 4, 4, 4, 4> doc;
// ... populate with instances ...

// Generate code
LVGLRenderer<16384> renderer;
renderer.generate(doc);

// Output
std::cout << renderer.c_str();
```

## Documentation

- [LVGL_RENDERER.md](LVGL_RENDERER.md) - Complete API documentation and usage guide
- [ANIMATION_IMPLEMENTATION.md](ANIMATION_IMPLEMENTATION.md) - Animation system implementation details
- [ASSET_BUNDLING.md](ASSET_BUNDLING.md) - Asset bundling with forma:// URI support
- [HIERARCHICAL_WIDGETS.md](HIERARCHICAL_WIDGETS.md) - Hierarchical widget tree implementation

## License

Same as Forma core library.

## Development

This plugin is designed to be self-contained and can be moved to its own repository.
All dependencies are on the Forma core library (tokenizer, parser, IR).
