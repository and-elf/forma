#pragma once

#include "toml/toml.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>

namespace forma {

// Plugin metadata loaded from plugin.toml
struct PluginMetadata {
    // [plugin]
    std::string name;
    std::string kind;  // "renderer", "build", "release", "target", "lsp", etc.
    std::string api_version;
    std::string runtime;  // "native", "js", "wasm", etc.
    std::string entrypoint;  // For script-based runtimes
    
    // [capabilities]
    std::vector<std::string> provides;  // e.g., "renderer:js", "widgets", "animation"
    std::vector<std::string> dependencies;  // Dependencies (renamed from requires to avoid keyword)
    
    // [renderer] (optional, for renderer plugins)
    std::string output_extension;  // e.g., ".c", ".cpp", ".js"
    std::string output_language;   // e.g., "c", "cpp", "javascript"
    
    // Helper methods
    bool is_renderer() const { return kind == "renderer"; }
    bool is_build() const { return kind == "build"; }
    bool is_lsp() const { return kind == "lsp"; }
    
    bool has_capability(const std::string& cap) const {
        for (const auto& p : provides) {
            if (p == cap) return true;
        }
        return false;
    }
    
    bool provides_renderer(const std::string& renderer_name) const {
        std::string cap = "renderer:" + renderer_name;
        return has_capability(cap);
    }
};

// Load plugin metadata from a TOML string (embedded in plugin binary)
inline std::unique_ptr<PluginMetadata> load_plugin_metadata_from_string(const char* toml_str) {
    if (!toml_str) {
        return nullptr;
    }
    
    std::string content(toml_str);
    if (content.empty()) {
        return nullptr;
    }
    
    // Parse TOML
    try {
        auto doc = forma::toml::parse(content);
        
        auto metadata = std::make_unique<PluginMetadata>();
    
    // Get [plugin] section
    const auto* plugin_table = doc.get_table("plugin");
    if (plugin_table) {
        if (auto name = plugin_table->get_string("name")) {
            metadata->name = std::string(*name);
        }
        if (auto kind = plugin_table->get_string("kind")) {
            metadata->kind = std::string(*kind);
        }
        if (auto api_version = plugin_table->get_string("api_version")) {
            metadata->api_version = std::string(*api_version);
        }
        if (auto runtime = plugin_table->get_string("runtime")) {
            metadata->runtime = std::string(*runtime);
        }
        if (auto entrypoint = plugin_table->get_string("entrypoint")) {
            metadata->entrypoint = std::string(*entrypoint);
        }
    }
    
    // Get [capabilities] section
    const auto* cap_table = doc.get_table("capabilities");
    if (cap_table) {
        // Get "provides" array
        if (const auto* provides_val = cap_table->get("provides")) {
            if (provides_val->type == forma::toml::ValueType::Array) {
                const auto& arr = doc.arrays[provides_val->array_index];
                for (size_t i = 0; i < arr.count; ++i) {
                    metadata->provides.push_back(std::string(arr.elements[i]));
                }
            }
        }
        
        // Get "requires" array (stored as dependencies)
        if (const auto* requires_val = cap_table->get("requires")) {
            if (requires_val->type == forma::toml::ValueType::Array) {
                const auto& arr = doc.arrays[requires_val->array_index];
                for (size_t i = 0; i < arr.count; ++i) {
                    metadata->dependencies.push_back(std::string(arr.elements[i]));
                }
            }
        }
    }
    
    // Get [renderer] section (optional)
    const auto* renderer_table = doc.get_table("renderer");
    if (renderer_table) {
        if (auto ext = renderer_table->get_string("output_extension")) {
            metadata->output_extension = std::string(*ext);
        }
        if (auto lang = renderer_table->get_string("output_language")) {
            metadata->output_language = std::string(*lang);
        }
    }
    
    return metadata;
    } catch (...) {
        // If TOML parsing fails, return nullptr
        return nullptr;
    }
}

// Load plugin metadata from a plugin.toml file
inline std::unique_ptr<PluginMetadata> load_plugin_metadata(const std::filesystem::path& toml_path) {
    if (!std::filesystem::exists(toml_path)) {
        return nullptr;
    }
    
    // Read the TOML file
    std::ifstream file(toml_path);
    if (!file) {
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    if (content.empty()) {
        return nullptr;
    }
    
    return load_plugin_metadata_from_string(content.c_str());
}

// Find plugin.toml for a given plugin path
// For /path/to/plugin.so, looks for /path/to/plugin.toml
inline std::filesystem::path find_plugin_toml(const std::filesystem::path& plugin_path) {
    auto dir = plugin_path.parent_path();
    auto stem = plugin_path.stem();  // filename without extension
    
    // Try same directory with .toml extension
    auto toml_path = dir / (stem.string() + ".toml");
    if (std::filesystem::exists(toml_path)) {
        return toml_path;
    }
    
    // Try plugin.toml in same directory
    toml_path = dir / "plugin.toml";
    if (std::filesystem::exists(toml_path)) {
        return toml_path;
    }
    
    return "";
}

} // namespace forma
