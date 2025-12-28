// JSON Renderer Plugin
// Exports Forma documents as JSON for web/debugging

#include <cstdint>
#include <iostream>

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

// Plugin descriptor
static FormaPluginDescriptor json_renderer = {
    .api_version = 1,
    .name = "json-renderer",
    .version = "1.0.0",
    .capabilities = {
        .supports_renderer = true,  // This is a renderer
        .supports_theme = false,
        .supports_audio = false,
        .supports_build = false
    },
    .register_plugin = nullptr
};

// Plugin initialization
FormaPluginDescriptor* forma_get_plugin_descriptor() {
    std::cout << "[JSON Renderer] Plugin loaded\n";
    std::cout << "[JSON Renderer] Provides JSON output for Forma documents\n";
    return &json_renderer;
}

} // extern "C"
