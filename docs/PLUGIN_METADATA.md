# TOML-Based Plugin Metadata System

## Overview

Plugins can now declare their capabilities through a `plugin.toml` configuration file instead of hardcoding them in the plugin binary. This makes the plugin system more flexible and allows the LSP to dynamically discover what types and capabilities are available.

## Architecture

### Plugin Metadata Structure

```cpp
struct PluginMetadata {
    // [plugin]
    std::string name;
    std::string kind;  // "renderer", "build", "release", "target", "lsp", etc.
    std::string api_version;
    std::string runtime;  // "native", "js", "wasm", etc.
    std::string entrypoint;  // For script-based runtimes
    
    // [capabilities]
    std::vector<std::string> provides;  // e.g., "renderer:js", "widgets", "animation"
    std::vector<std::string> dependencies;
    
    // [renderer] (optional)
    std::string output_extension;  // e.g., ".c", ".cpp", ".js"
    std::string output_language;   // e.g., "c", "cpp", "javascript"
};
```

### TOML Format

```toml
[plugin]
name = "lvgl"
kind = "renderer"
api_version = "1.0.0"
runtime = "native"  # or "js", "wasm", etc.
entrypoint = "index.js"  # only for script-based runtimes

[capabilities]
provides = [
    "renderer:lvgl",
    "renderer:c",
    "widgets:basic",
    "widgets:lvgl",
    "animation",
    "events",
    "layouts"
]

dependencies = []

[renderer]
output_extension = ".c"
output_language = "c"
```

## Plugin Discovery

### For Dynamic Plugins (.so files)

When loading a plugin from a path like `/path/to/forma-renderer.so`, the loader looks for:
1. `/path/to/forma-renderer.toml`
2. `/path/to/plugin.toml`

### For Built-in Plugins

Built-in plugins can optionally provide metadata when registering:

```cpp
auto metadata = forma::load_plugin_metadata("plugins/lvgl-renderer/plugin.toml");
plugin_loader.register_builtin_plugin(
    descriptor,
    "lvgl",
    std::move(metadata)
);
```

## Capability System

### String-Based Capabilities

Instead of boolean flags like `supports_renderer`, plugins declare capabilities as strings:

- `renderer:lvgl` - Provides LVGL rendering
- `renderer:js` - Provides JavaScript rendering
- `widgets:basic` - Provides basic widgets (Button, Label, etc.)
- `widgets:lvgl` - Provides LVGL-specific widgets
- `animation` - Supports animations
- `events` - Supports event handling
- `layouts` - Supports layout containers

### LSP Integration

The LSP can query all loaded plugins and collect their capabilities to determine:
- What widget types are available
- What properties are valid for each renderer
- What animation types are supported
- What event handlers can be attached

Example:
```cpp
if (plugin->metadata && plugin->metadata->has_capability("widgets:lvgl")) {
    // Enable lvgl-specific widget autocomplete
}
```

## Benefits

### 1. **Flexibility**
- No need to recompile plugins to change capabilities
- Easy to add new capability types

### 2. **LSP Type Resolution**
- LSP can determine valid types based on loaded plugins
- Renderer-specific widgets only appear when that renderer is selected
- Property validation based on renderer capabilities

### 3. **Plugin Kinds**
Different plugin types for different purposes:
- `renderer` - Code generators (C, C++, JS, etc.)
- `build` - Build system integrators (CMake, Meson, etc.)
- `lsp` - Language server extensions
- `target` - Target platform support (embedded, web, mobile)
- `release` - Distribution/packaging tools

### 4. **Runtime Diversity**
- `native` - Compiled shared libraries (.so, .dll)
- `js` - JavaScript plugins (via V8/QuickJS)
- `wasm` - WebAssembly plugins
- `lua` - Lua scripts

## Current Status

### ✅ Implemented
- Plugin metadata loading infrastructure
- TOML parser integration
- LoadedPlugin includes metadata field
- Fallback to descriptor if no metadata
- Plugin listing shows capabilities from metadata

### 🚧 In Progress
- TOML parser has issues (currently disabled)
- Need to debug/fix constexpr TOML parser for runtime use

### 📋 TODO
- Fix TOML parser infinite loop
- Enable metadata loading for built-in LVGL plugin
- Create plugin.toml for CMake generator
- Update LSP to use capability strings for type resolution
- Add validation for required dependencies
- Implement script-based plugin loading (js/lua runtime)

## Files Changed

- `src/plugin_metadata.hpp` (NEW) - Metadata loading and types
- `src/plugin_loader.hpp` - Updated to include metadata
- `forma.cpp` - Uses metadata for output extension
- `plugins/lvgl-renderer/plugin.toml` (NEW) - LVGL capabilities

## Example Plugin.toml Files

### JavaScript Renderer
```toml
[plugin]
name = "js-renderer"
kind = "renderer"
api_version = "1.0.0"
runtime = "native"

[capabilities]
provides = [
    "renderer:js",
    "renderer:react",
    "widgets:web",
    "animation",
    "events:dom"
]

[renderer]
output_extension = ".jsx"
output_language = "javascript"
```

### LSP Extension
```toml
[plugin]
name = "forma-lsp-advanced"
kind = "lsp"
api_version = "1.0.0"
runtime = "native"

[capabilities]
provides = [
    "lsp:goto-definition",
    "lsp:find-references",
    "lsp:rename",
    "lsp:semantic-tokens"
]
```

## Migration Path

### Phase 1: Coexistence (Current)
- Both descriptor capabilities and TOML metadata supported
- Metadata takes precedence when available
- Fallback to descriptor for backward compatibility

### Phase 2: Transition
- All built-in plugins have plugin.toml
- LSP uses metadata for type resolution
- Documentation updated

### Phase 3: Deprecation
- Descriptor capabilities marked deprecated
- Warning for plugins without metadata

### Phase 4: Removal
- Remove boolean capability flags from descriptor
- Metadata becomes mandatory for all plugins
