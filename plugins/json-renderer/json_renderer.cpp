// JSON Renderer Plugin
// Exports Forma documents as JSON for web/debugging

#include <parser/ir.hpp>
#include "../../src/plugin_hash.hpp"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>

// Plugin metadata hash - computed from plugin.toml at compile time
// This ensures the TOML file matches the binary
constexpr std::string_view PLUGIN_TOML_CONTENT = R"(# JSON Renderer Plugin

[plugin]
name = "json-renderer"
kind = "renderer"
api_version = "1.0.0"
runtime = "native"

[capabilities]
provides = [
    "renderer:json",
    "export:json",
    "serialization",
    "debugging"
]

dependencies = []

[renderer]
output_extension = ".json"
output_language = "json"
)";

constexpr uint64_t METADATA_HASH = forma::fnv1a_hash(PLUGIN_TOML_CONTENT);

// Plugin exports
extern "C" {

// Plugin metadata hash (required) - for verification
uint64_t forma_plugin_metadata_hash() {
    return METADATA_HASH;
}

// Render function (required) - exports Forma document as JSON
bool forma_render(const void* doc_ptr, const char* input_path, const char* output_path) {
    (void)input_path; // Unused
    
    if (!doc_ptr || !output_path) {
        std::cerr << "[JSON Renderer] Error: null pointer passed to render\n";
        return false;
    }
    
    try {
        // Cast the document pointer
        const auto* doc = static_cast<const forma::Document<32,16,16,32,64,64>*>(doc_ptr);
        
        std::ostringstream json;
        json << "{\n";
        json << "  \"types\": [\n";
        
        // Export types
        for (size_t i = 0; i < doc->type_count; ++i) {
            const auto& type = doc->types[i];
            json << "    {\n";
            json << "      \"name\": \"" << std::string(type.name.data(), type.name.size()) << "\",\n";
            json << "      \"base\": \"" << std::string(type.base_type.data(), type.base_type.size()) << "\",\n";
            json << "      \"property_count\": " << type.prop_count << ",\n";
            json << "      \"method_count\": " << type.method_count << "\n";
            json << "    }";
            if (i < doc->type_count - 1) json << ",";
            json << "\n";
        }
        
        json << "  ],\n";
        json << "  \"instances\": [\n";
        
        // Export instances
        for (size_t i = 0; i < doc->instances.count; ++i) {
            const auto& inst = doc->instances.instances[i];
            json << "    {\n";
            json << "      \"type\": \"" << std::string(inst.type_name.data(), inst.type_name.size()) << "\",\n";
            json << "      \"property_count\": " << inst.prop_count << ",\n";
            json << "      \"child_count\": " << inst.child_count << "\n";
            json << "    }";
            if (i < doc->instances.count - 1) json << ",";
            json << "\n";
        }
        
        json << "  ],\n";
        json << "  \"type_count\": " << doc->type_count << ",\n";
        json << "  \"instance_count\": " << doc->instances.count << ",\n";
        json << "  \"enum_count\": " << doc->enum_count << ",\n";
        json << "  \"import_count\": " << doc->import_count << "\n";
        json << "}\n";
        
        // Write output file
        std::ofstream out(output_path);
        if (!out) {
            std::cerr << "[JSON Renderer] Error: cannot write to " << output_path << "\n";
            return false;
        }
        
        out << json.str();
        out.close();
        
        std::cout << "[JSON Renderer] Generated " << json.str().size() 
                  << " bytes to " << output_path << "\n";
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[JSON Renderer] Error: " << e.what() << "\n";
        return false;
    }
}

// Optional registration function
void forma_register(void* host) {
    (void)host; // Unused for now
    // NOTE: Don't print here - this may be called during plugin loading
}

} // extern "C"
