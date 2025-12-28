# LVGL Renderer - Dual Mode Support

The LVGL renderer can be used in two ways:

## 1. Built-in Mode (Default)

The LVGL renderer is compiled directly into the `forma` binary as a header-only library.

```bash
# No plugin needed - just works
./forma input.forma
```

**Advantages:**
- No plugin loading required
- Faster (no dlopen overhead)
- Always available
- Good for simple use cases

## 2. Plugin Mode

The LVGL renderer can also be loaded as a dynamic plugin.

```bash
# Build the plugin
g++ -std=c++20 -fPIC -shared -I. \
    plugins/lvgl-renderer/src/lvgl_renderer_plugin.cpp \
    -o plugins/lvgl-renderer/liblvgl_renderer.so -Wl,-E

# Use as plugin
./forma --plugin plugins/lvgl-renderer/liblvgl_renderer.so --list-plugins
```

**Advantages:**
- Can be versioned independently
- Updatable without recompiling forma
- Consistent with other renderer plugins
- Better for production deployments

## Current Status

✅ **Plugin infrastructure** - LVGL renderer has `forma_get_plugin_descriptor`  
✅ **Plugin loading** - Can be loaded via `--plugin` flag  
✅ **Built-in fallback** - Works without plugin for convenience  
⏳ **Plugin invocation** - Render callback not yet wired up (uses built-in currently)

## Files

- `plugins/lvgl-renderer/src/lvgl_renderer.hpp` - Header-only renderer (used by built-in)
- `plugins/lvgl-renderer/src/lvgl_renderer_plugin.cpp` - Plugin wrapper
- `plugins/lvgl-renderer/liblvgl_renderer.so` - Compiled plugin

## Next Steps

To fully switch to plugin mode, we need to:
1. Add render callback to `FormaPluginDescriptor`
2. Wire up plugin invocation in `forma.cpp`
3. Optionally remove built-in fallback
