// Example Plugin: Hello World
// Demonstrates dynamic plugin loading

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
static FormaPluginDescriptor hello_plugin = {
    .api_version = 1,
    .name = "hello-world",
    .version = "1.0.0",
    .capabilities = {
        .supports_renderer = false,
        .supports_theme = false,
        .supports_audio = false,
        .supports_build = false
    },
    .register_plugin = nullptr
};

// Plugin initialization function (must be exported)
FormaPluginDescriptor* forma_get_plugin_descriptor() {
    std::cout << "[Hello Plugin] Initializing...\n";
    return &hello_plugin;
}

} // extern "C"
