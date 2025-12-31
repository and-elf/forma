#pragma once

#include <plugin_hash.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace forma::plugin {

/**
 * Read a TOML file and compute its hash for plugin metadata verification.
 * This ensures plugins use their actual forma.toml file as single source of truth.
 * 
 * @param toml_path Path to the forma.toml file (relative to plugin source)
 * @param plugin_name Name of the plugin (for error messages)
 * @return Hash of the TOML file content
 */
inline uint64_t read_and_hash_toml(const char* toml_path, const char* plugin_name) {
    std::ifstream file(toml_path);
    if (!file) {
        std::cerr << "[" << plugin_name << "] Warning: Could not read " << toml_path << "\n";
        return 0;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    return forma::fnv1a_hash(content);
}

/**
 * Macro to simplify TOML path stringification in plugins.
 * Usage: FORMA_PLUGIN_TOML_HASH("plugin-name", ../forma.toml)
 */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FORMA_PLUGIN_TOML_HASH(name, path) forma::plugin::read_and_hash_toml(TOSTRING(path), name)

} // namespace forma::plugin
