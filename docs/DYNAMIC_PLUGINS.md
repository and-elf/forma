# Dynamic Plugin Loading System

The Forma compiler now supports loading plugins dynamically at runtime from shared libraries (.so files).

## Quick Start

```bash
# Build a plugin
g++ -std=c++20 -fPIC -shared -Wl,-E my_plugin.cpp -o libmyplugin.so

# Use the plugin
./forma --plugin ./libmyplugin.so input.forma

# List loaded plugins
./forma --plugin ./libmyplugin.so --list-plugins
```

## Features

✅ **Dynamic Loading** - Load plugins at runtime with `dlopen`  
✅ **Version Checking** - Validates API compatibility  
✅ **Multiple Plugins** - Load multiple plugins simultaneously  
✅ **Capability Detection** - Plugins declare what they support  
✅ **Error Reporting** - Clear error messages for loading failures  
✅ **Plugin Listing** - `--list-plugins` shows all loaded plugins

## Implementation

### Core Components

- **[src/plugin_loader.hpp](src/plugin_loader.hpp)** - PluginLoader class
- **[src/plugin.hpp](src/plugin.hpp)** - Plugin interface definitions (stub)
- **[forma.cpp](forma.cpp)** - Integration with compiler

### Plugin Loader Class

```cpp
class PluginLoader {
public:
    bool load_plugin(const std::string& path, std::string& error_msg);
    LoadedPlugin* find_plugin(const std::string& name);
    void print_loaded_plugins(std::ostream& out);
    const std::vector<LoadedPlugin*>& get_loaded_plugins();
};
```

### Plugin Interface

Every plugin must export:
```cpp
extern "C" {
    FormaPluginDescriptor* forma_get_plugin_descriptor();
}
```

## Example Plugins

### 1. Hello World Plugin

Location: `plugins/hello-world/`

Minimal plugin demonstrating the loading system.

```cpp
static FormaPluginDescriptor hello_plugin = {
    .api_version = 1,
    .name = "hello-world",
    .version = "1.0.0",
    .capabilities = {...},
    .register_plugin = nullptr
};

extern "C" {
    FormaPluginDescriptor* forma_get_plugin_descriptor() {
        return &hello_plugin;
    }
}
```

**Build:**
```bash
g++ -std=c++20 -fPIC -shared -Wl,-E hello_plugin.cpp -o libhello.so
```

**Use:**
```bash
./forma --plugin plugins/hello-world/libhello.so --list-plugins
```

**Output:**
```
[Hello Plugin] Initializing...
Loaded plugins:
  - hello-world v1.0.0
    Path: plugins/hello-world/libhello.so
```

### 2. JSON Renderer Plugin

Location: `plugins/json-renderer/`

Renderer plugin for JSON output.

**Capabilities:** renderer ✓, theme ✗, audio ✗, build ✗

**Build:**
```bash
g++ -std=c++20 -fPIC -shared -Wl,-E json_renderer.cpp -o libjson_renderer.so
```

**Use:**
```bash
./forma --plugin plugins/json-renderer/libjson_renderer.so --list-plugins
```

**Output:**
```
[JSON Renderer] Plugin loaded
[JSON Renderer] Provides JSON output for Forma documents
Loaded plugins:
  - json-renderer v1.0.0 [renderer]
    Path: plugins/json-renderer/libjson_renderer.so
```

## Loading Multiple Plugins

```bash
./forma \
  --plugin plugins/hello-world/libhello.so \
  --plugin plugins/json-renderer/libjson_renderer.so \
  --list-plugins
```

Output:
```
[Hello Plugin] Initializing...
[JSON Renderer] Plugin loaded
[JSON Renderer] Provides JSON output for Forma documents
Loaded plugins:
  - hello-world v1.0.0
    Path: plugins/hello-world/libhello.so
  - json-renderer v1.0.0 [renderer]
    Path: plugins/json-renderer/libjson_renderer.so
```

## Command Line Options

- `--plugin <path>` - Load a plugin from shared library
- `--list-plugins` - Display all loaded plugins
- `-v, --verbose` - Show plugin loading details

## Plugin Development

See [Plugin Development Guide](docs/PLUGIN_DEVELOPMENT.md) for complete documentation.

### Minimal Plugin Template

```cpp
#include <cstdint>

extern "C" {

struct PluginCapabilities {
    bool supports_renderer;
    bool supports_theme;
    bool supports_audio;
    bool supports_build;
};

struct FormaPluginDescriptor {
    uint32_t api_version;
    const char* name;
    const char* version;
    PluginCapabilities capabilities;
    void* register_plugin;
};

static FormaPluginDescriptor my_plugin = {
    .api_version = 1,
    .name = "my-plugin",
    .version = "1.0.0",
    .capabilities = {false, false, false, false},
    .register_plugin = nullptr
};

FormaPluginDescriptor* forma_get_plugin_descriptor() {
    return &my_plugin;
}

} // extern "C"
```

## Build Flags

**Required flags for plugins:**
- `-std=c++20` - C++20 standard
- `-fPIC` - Position Independent Code
- `-shared` - Create shared library
- `-Wl,-E` or `-Wl,--export-dynamic` - Export symbols

**Example:**
```bash
g++ -std=c++20 -fPIC -shared -Wl,-E plugin.cpp -o libplugin.so
```

## Verifying Symbol Export

Check that `forma_get_plugin_descriptor` is exported:
```bash
nm -D libplugin.so | grep forma
```

Should output:
```
0000000000001109 T forma_get_plugin_descriptor
```

## Error Messages

### "Cannot find forma_get_plugin_descriptor"

**Cause:** Symbol not exported correctly

**Solution:**
1. Use `extern "C"` around function
2. Build with `-Wl,-E` or `-Wl,--export-dynamic`
3. Verify with `nm -D libplugin.so`

### "Incompatible API version"

**Cause:** Plugin uses wrong API version

**Solution:** Set `api_version = 1` in descriptor

### "Failed to load plugin: cannot open shared object file"

**Cause:** File not found or not readable

**Solution:**
1. Check file path is correct
2. Ensure file is readable: `chmod +r libplugin.so`
3. Use absolute paths

## Architecture

```
┌─────────────┐
│   forma     │
│  (binary)   │
└──────┬──────┘
       │
       │ --plugin libfoo.so
       ├──────────────────────────────┐
       │                              │
       v                              v
┌──────────────┐            ┌──────────────────┐
│ PluginLoader │            │  libfoo.so       │
│              │  dlopen()  │                  │
│ load_plugin()├───────────>│ forma_get_plugin_│
│              │            │    descriptor()  │
│ find_plugin()│  dlsym()   │                  │
│              ├───────────>│ Returns          │
│ list()       │            │ FormaPlugin-     │
└──────────────┘            │    Descriptor*   │
                            └──────────────────┘
```

## Testing

```bash
# Build compiler with plugin support
g++ -std=c++20 -Wall -Wextra forma.cpp -o forma -ldl

# Build test plugins
cd plugins/hello-world
g++ -std=c++20 -fPIC -shared -Wl,-E hello_plugin.cpp -o libhello.so

# Test loading
./forma --plugin plugins/hello-world/libhello.so --list-plugins

# Test with compilation
./forma --plugin plugins/hello-world/libhello.so examples/simple_hierarchy.forma
```

## Future Enhancements

- [ ] Hot reload support
- [ ] Plugin configuration files (TOML/JSON)
- [ ] Plugin dependency resolution
- [ ] Plugin marketplace/registry
- [ ] Windows DLL support
- [ ] macOS dylib support
- [ ] Plugin sandboxing
- [ ] Plugin API documentation generator

## Platform Support

| Platform | Library Format | Status |
|----------|---------------|--------|
| Linux    | `.so`         | ✅ Supported |
| Windows  | `.dll`        | ⏳ Planned |
| macOS    | `.dylib`      | ⏳ Planned |

## Related Documentation

- [Plugin Development Guide](docs/PLUGIN_DEVELOPMENT.md)
- [Hello World Plugin](plugins/hello-world/README.md)
- [JSON Renderer Plugin](plugins/json-renderer/README.md)
