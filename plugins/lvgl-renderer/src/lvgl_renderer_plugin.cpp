// LVGL Renderer Plugin Wrapper
// Makes the LVGL renderer available as a dynamic plugin

#include "lvgl_renderer.hpp"
#include <plugin_utils.hpp>
#include <cstdint>
#include <iostream>
#include <fstream>

// Plugin metadata - computed from forma.toml (single source of truth)
static const uint64_t METADATA_HASH = FORMA_PLUGIN_TOML_HASH("LVGL Renderer", ../forma.toml);

// Plugin exports
extern "C" {

// Plugin metadata hash (required)
uint64_t forma_plugin_metadata_hash() {
    return METADATA_HASH;
}

// Render function (required)
bool forma_render(const void* doc_ptr, const char* input_path, const char* output_path) {
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
