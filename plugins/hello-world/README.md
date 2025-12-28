# Hello World Plugin

A minimal example demonstrating Forma's dynamic plugin loading system.

## Building

```bash
g++ -std=c++20 -fPIC -shared -Wl,--export-dynamic hello_plugin.cpp -o libhello.so
```

## Using

```bash
# Load the plugin
./forma --plugin plugins/hello-world/libhello.so --list-plugins

# Use with compilation
./forma --plugin plugins/hello-world/libhello.so input.forma
```

## Plugin Interface

Plugins must export a `forma_get_plugin_descriptor()` function that returns a `FormaPluginDescriptor*`:

```cpp
extern "C" {
    FormaPluginDescriptor* forma_get_plugin_descriptor();
}
```

The descriptor contains:
- `api_version` - Must be `1`
- `name` - Plugin name
- `version` - Plugin version
- `capabilities` - What the plugin supports (renderer, theme, audio, build)
- `register_plugin` - Optional registration function

## Output

```
[Hello Plugin] Initializing...
Loaded plugins:
  - hello-world v1.0.0
    Path: plugins/hello-world/libhello.so
```
