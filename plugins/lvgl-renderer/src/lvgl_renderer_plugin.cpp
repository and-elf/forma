// LVGL Renderer Plugin Wrapper
// Makes the LVGL renderer available as a dynamic plugin

#include "lvgl_renderer.hpp"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cstring>

// Plugin exports - all metadata is in plugin.toml
extern "C" {

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
