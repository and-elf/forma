// LVGL Renderer Plugin Wrapper
// Makes the LVGL renderer available as a dynamic plugin

#include "lvgl_renderer.hpp"
#include <plugin_hash.hpp>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

// Plugin metadata - read from forma.toml to ensure single source of truth
// Path is relative to the plugin source directory
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef FORMA_TOML_PATH
#define FORMA_TOML_PATH ../forma.toml
#endif

static std::string read_toml_file() {
    const char* toml_path = TOSTRING(FORMA_TOML_PATH);
    std::ifstream file(toml_path);
    if (!file) {
        std::cerr << "[LVGL Renderer] Warning: Could not read " << toml_path << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static const std::string PLUGIN_TOML_CONTENT = read_toml_file();
static const uint64_t METADATA_HASH = forma::fnv1a_hash(PLUGIN_TOML_CONTENT);

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
