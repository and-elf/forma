// LVGL Renderer Plugin Wrapper
// Makes the LVGL renderer available as a dynamic plugin

#include "lvgl_renderer.hpp"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cstring>

extern "C" {

struct PluginCapabilities {
    bool supports_renderer;
    bool supports_theme;
    bool supports_audio;
    bool supports_build;
    
    // Renderer callback: render(doc, input_path, output_path) -> success
    bool (*render)(const void* doc, const char* input_path, const char* output_path);
};

struct FormaPluginDescriptor {
    uint32_t api_version;
    const char* name;
    const char* version;
    PluginCapabilities capabilities;
    void* register_plugin;
};

// Forward declare render function
static bool lvgl_render(const void* doc_ptr, const char* input_path, const char* output_path);

// Plugin descriptor
static FormaPluginDescriptor lvgl_plugin = {
    .api_version = 1,
    .name = "lvgl-renderer",
    .version = "1.0.0",
    .capabilities = {
        .supports_renderer = true,
        .supports_theme = false,
        .supports_audio = false,
        .supports_build = false,
        .render = lvgl_render
    },
    .register_plugin = nullptr
};

// Plugin initialization
FormaPluginDescriptor* forma_get_plugin_descriptor() {
    std::cout << "[LVGL Renderer] Plugin loaded\n";
    std::cout << "[LVGL Renderer] C99 code generator for LVGL UI framework\n";
    return &lvgl_plugin;
}

// Render callback implementation
static bool lvgl_render(const void* doc_ptr, const char* input_path, const char* output_path) {
    (void)input_path; // Unused
    
    if (!doc_ptr || !output_path) {
        std::cerr << "[LVGL Renderer] Error: null pointer passed to render\n";
        return false;
    }
    
    try {
        // Cast the document pointer
        const auto* doc = static_cast<const forma::Document<32,16,16,32,64,64>*>(doc_ptr);
        
        // Create renderer
        forma::lvgl::LVGLRenderer<65536> renderer;
        
        // Generate code
        renderer.generate(*doc);
        
        // Write output file
        std::ofstream out(output_path);
        if (!out) {
            std::cerr << "[LVGL Renderer] Error: cannot write to " << output_path << "\n";
            return false;
        }
        
        out << renderer.get_output();
        out.close();
        
        std::cout << "[LVGL Renderer] Generated " << renderer.get_output().size() 
                  << " bytes to " << output_path << "\n";
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LVGL Renderer] Error: " << e.what() << "\n";
        return false;
    }
}

} // extern "C"
