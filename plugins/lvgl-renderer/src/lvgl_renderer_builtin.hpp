#pragma once

#include "lvgl_renderer.hpp"
#include "../../src/plugin_loader.hpp"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cstring>

namespace forma::lvgl {

// Render callback implementation for built-in plugin
static bool lvgl_builtin_render(const void* doc_ptr, const char* input_path, const char* output_path) {
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

// Built-in plugin descriptor
static forma::FormaPluginDescriptor lvgl_builtin_descriptor = {
    .api_version = 1,
    .name = "lvgl",
    .version = "1.0.0-builtin",
    .capabilities = {
        .supports_renderer = true,
        .supports_theme = false,
        .supports_audio = false,
        .supports_build = false,
        .render = lvgl_builtin_render,
        .output_extension = ".c"
    },
    .register_plugin = nullptr
};

// Get the built-in plugin descriptor
inline forma::FormaPluginDescriptor* get_builtin_descriptor() {
    return &lvgl_builtin_descriptor;
}

} // namespace forma::lvgl
