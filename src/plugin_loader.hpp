#pragma once

#include <dlfcn.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <memory>
#include <filesystem>
#include "plugin_metadata.hpp"
#include "plugin_hash.hpp"

namespace forma {

// Plugin function pointers loaded from dynamic library
struct PluginFunctions {
    bool (*render)(const void* doc, const char* input_path, const char* output_path);
    void (*register_plugin)(void* host);  // Optional
    uint64_t (*get_metadata_hash)();  // Required - returns hash of expected TOML
};

struct LoadedPlugin {
    void* handle = nullptr;
    PluginFunctions functions;
    std::string path;
    std::unique_ptr<PluginMetadata> metadata;  // Loaded from plugin.toml
    
    ~LoadedPlugin() {
        if (handle) {
            dlclose(handle);
        }
    }
};

class PluginLoader {
private:
    std::vector<std::unique_ptr<LoadedPlugin>> loaded_plugins;
    std::vector<std::string> plugin_search_paths;  // Custom search paths
    
public:
    ~PluginLoader() = default;
    
    // Load a plugin from a shared library (.so file)
    bool load_plugin(const std::string& path, std::string& error_msg) {
        // Open the shared library
        void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle) {
            error_msg = std::string("Failed to load plugin: ") + dlerror();
            return false;
        }
        
        // Clear any existing error
        dlerror();
        
        // Look for the forma_render function (required)
        typedef bool (*RenderFunc)(const void*, const char*, const char*);
        RenderFunc render_fn = reinterpret_cast<RenderFunc>(dlsym(handle, "forma_render"));
        
        const char* dlsym_error = dlerror();
        if (dlsym_error || !render_fn) {
            error_msg = std::string("Cannot find forma_render function: ") + (dlsym_error ? dlsym_error : "symbol not found");
            dlclose(handle);
            return false;
        }
        
        // Look for optional forma_register function
        typedef void (*RegisterFunc)(void*);
        RegisterFunc register_fn = reinterpret_cast<RegisterFunc>(dlsym(handle, "forma_register"));
        dlerror(); // Clear error (it's optional)
        
        // Look for forma_plugin_metadata_hash function (required)
        typedef uint64_t (*MetadataHashFunc)();
        MetadataHashFunc hash_fn = reinterpret_cast<MetadataHashFunc>(dlsym(handle, "forma_plugin_metadata_hash"));
        
        dlsym_error = dlerror();
        if (dlsym_error || !hash_fn) {
            error_msg = std::string("Cannot find forma_plugin_metadata_hash function: ") + (dlsym_error ? dlsym_error : "symbol not found");
            dlclose(handle);
            return false;
        }
        
        // Get expected hash from plugin
        uint64_t expected_hash = hash_fn();
        
        // Try to load plugin.toml from same directory as .so
        auto toml_path = find_plugin_toml(path);
        if (toml_path.empty()) {
            error_msg = "Plugin metadata file (plugin.toml) not found for: " + path;
            dlclose(handle);
            return false;
        }
        
        // Load and verify metadata
        auto metadata = load_plugin_metadata(toml_path);
        if (!metadata) {
            error_msg = "Failed to parse plugin metadata from: " + toml_path.string();
            dlclose(handle);
            return false;
        }
        
        // Verify hash matches
        std::ifstream toml_file(toml_path);
        std::stringstream toml_buffer;
        toml_buffer << toml_file.rdbuf();
        std::string toml_content = toml_buffer.str();
        uint64_t actual_hash = forma::fnv1a_hash(toml_content);
        
        if (actual_hash != expected_hash) {
            error_msg = std::string("Plugin metadata hash mismatch!\n") +
                       "  Expected: " + forma::hash_to_hex(expected_hash) + "\n" +
                       "  Got:      " + forma::hash_to_hex(actual_hash) + "\n" +
                       "  TOML file may be outdated or corrupted: " + toml_path.string();
            dlclose(handle);
            return false;
        }
        
        // Validate API version from metadata
        if (metadata->api_version != "1.0.0") {
            error_msg = std::string("Incompatible API version: expected 1.0.0, got ") + metadata->api_version;
            dlclose(handle);
            return false;
        }
        
        // Store the loaded plugin
        auto loaded = std::make_unique<LoadedPlugin>();
        loaded->handle = handle;
        loaded->functions.render = render_fn;
        loaded->functions.register_plugin = register_fn;
        loaded->functions.get_metadata_hash = hash_fn;
        loaded->path = path;
        loaded->metadata = std::move(metadata);
        loaded_plugins.push_back(std::move(loaded));
        
        // Call registration function if available
        if (register_fn) {
            // TODO: Create FormaHost instance and pass it
            // For now, just acknowledge the plugin is loaded
        }
        
        return true;
    }
    
    // Load all plugins from a directory
    // Each .so must have a matching plugin.toml in the same directory
    int load_plugins_from_directory(const std::string& dir_path, std::vector<std::string>& errors) {
        std::vector<std::string> plugin_files;
        
        // Find all .so files in directory
        try {
            for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".so") {
                    plugin_files.push_back(entry.path().string());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            errors.push_back(std::string("Failed to read directory: ") + e.what());
            return 0;
        }
        
        // Load each plugin
        int loaded_count = 0;
        for (const auto& plugin_path : plugin_files) {
            std::string error_msg;
            if (load_plugin(plugin_path, error_msg)) {
                loaded_count++;
            } else {
                errors.push_back(plugin_path + ": " + error_msg);
            }
        }
        
        return loaded_count;
    }
    
    // Load plugin by name from known plugin directories
    // Example: load_plugin_by_name("c-codegen") looks for "forma-c-codegen.so"
    bool load_plugin_by_name(const std::string& plugin_name, std::string& error_msg) {
        // Standard plugin search paths (in order of priority)
        std::vector<std::string> search_paths;
        
        // 0. User-added search paths (highest priority)
        search_paths.insert(search_paths.end(), plugin_search_paths.begin(), plugin_search_paths.end());
        
        // 1. Current directory (for development)
        search_paths.push_back(".");
        
        // 2. Relative to executable (for local installs)
        if (auto exe_path = std::filesystem::read_symlink("/proc/self/exe"); std::filesystem::exists(exe_path)) {
            auto plugin_dir = exe_path.parent_path() / "plugins";
            if (std::filesystem::exists(plugin_dir)) {
                search_paths.push_back(plugin_dir.string());
            }
        }
        
        // 3. System plugin directory
        search_paths.push_back("/usr/local/lib/forma/plugins");
        search_paths.push_back("/usr/lib/forma/plugins");
        
        // Try different naming conventions
        std::vector<std::string> name_variants = {
            "forma-" + plugin_name + ".so",
            "libforma-" + plugin_name + ".so",
            plugin_name + ".so"
        };
        
        // Search for plugin
        for (const auto& search_path : search_paths) {
            for (const auto& variant : name_variants) {
                auto full_path = std::filesystem::path(search_path) / variant;
                if (std::filesystem::exists(full_path)) {
                    return load_plugin(full_path.string(), error_msg);
                }
            }
        }
        
        error_msg = "Plugin '" + plugin_name + "' not found in standard directories";
        return false;
    }
    
    // Add a directory to the plugin search path
    void add_plugin_search_path(const std::string& dir_path) {
        if (std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path)) {
            plugin_search_paths.push_back(dir_path);
        }
    }
    
    // Register a built-in (statically-linked) plugin with metadata
    void register_builtin_plugin(
        bool (*render_fn)(const void*, const char*, const char*),
        void (*register_fn)(void*),
        std::unique_ptr<PluginMetadata> metadata) {
        
        if (!render_fn || !metadata) {
            std::cerr << "Warning: Built-in plugin missing render function or metadata\n";
            return;
        }
        
        // Validate API version
        if (metadata->api_version != "1.0.0") {
            std::cerr << "Warning: Built-in plugin " << metadata->name
                      << " has incompatible API version: " << metadata->api_version << std::endl;
            return;
        }
        
        // Create a LoadedPlugin entry without a dynamic library handle
        auto loaded = std::make_unique<LoadedPlugin>();
        loaded->handle = nullptr;
        loaded->functions.render = render_fn;
        loaded->functions.register_plugin = register_fn;
        loaded->path = "builtin:" + metadata->name;
        loaded->metadata = std::move(metadata);
        loaded_plugins.push_back(std::move(loaded));
    }
    
    // Get all loaded plugins
    const std::vector<std::unique_ptr<LoadedPlugin>>& get_loaded_plugins() const {
        return loaded_plugins;
    }
    
    // Get plugin by name
    LoadedPlugin* find_plugin(const std::string& name) {
        for (auto& plugin : loaded_plugins) {
            if (plugin->metadata && plugin->metadata->name == name) {
                return plugin.get();
            }
        }
        return nullptr;
    }
    
    // Print loaded plugins
    void print_loaded_plugins(std::ostream& out = std::cout) const {
        if (loaded_plugins.empty()) {
            out << "No plugins loaded\n";
            return;
        }
        
        out << "Loaded plugins:\n";
        for (const auto& plugin : loaded_plugins) {
            if (plugin->metadata) {
                out << "  - " << plugin->metadata->name 
                    << " v" << plugin->metadata->api_version;
                
                // Show capabilities from metadata
                if (!plugin->metadata->provides.empty()) {
                    out << " [" << plugin->metadata->kind << "]";
                    out << "\n    Provides: ";
                    for (size_t i = 0; i < plugin->metadata->provides.size(); ++i) {
                        out << plugin->metadata->provides[i];
                        if (i < plugin->metadata->provides.size() - 1) out << ", ";
                    }
                    if (!plugin->metadata->output_extension.empty()) {
                        out << "\n    Output: " << plugin->metadata->output_extension 
                            << " (" << plugin->metadata->output_language << ")";
                    }
                } else {
                    // No capabilities listed
                    out << " [" << plugin->metadata->kind << "]";
                }
                
                out << "\n    Path: " << plugin->path << "\n";
            }
        }
    }
};

} // namespace forma
