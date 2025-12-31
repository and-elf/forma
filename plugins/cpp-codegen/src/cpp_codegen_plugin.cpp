#include "cpp_codegen.hpp"
#include <plugin_hash.hpp>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>

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
        std::cerr << "[C++ Codegen] Warning: Could not read " << toml_path << "\n";
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
        std::cerr << "[C++ Codegen] Error: null pointer passed to render\n";
        return false;
    }
    
    try {
        // Cast the document pointer
        const auto* doc = static_cast<const forma::Document<32,16,16,32,64,64>*>(doc_ptr);
        
        // Create generator
        forma::codegen::CppCodeGenerator<65536> generator;
        
        // Generate code
        generator.generate(*doc);
        
        // Write output file
        std::ofstream out(output_path);
        if (!out) {
            std::cerr << "[C++ Codegen] Error: cannot write to " << output_path << "\n";
            return false;
        }
        
        out << generator.get_output();
        out.close();
        
        std::cout << "[C++ Codegen] Generated " << generator.get_output().size() 
                  << " bytes to " << output_path << "\n";
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[C++ Codegen] Exception: " << e.what() << "\n";
        return false;
    }
}

} // extern "C"
