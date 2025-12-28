#pragma once

#include <dlfcn.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <memory>
#include "plugin_metadata.hpp"

namespace forma {

// Plugin function pointers loaded from dynamic library
struct PluginFunctions {
    bool (*render)(const void* doc, const char* input_path, const char* output_path);
    void (*register_plugin)(void* host);  // Optional
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
        
        // Try to load plugin metadata from plugin.toml
        auto toml_path = find_plugin_toml(path);
        auto metadata = toml_path.empty() ? nullptr : load_plugin_metadata(toml_path);
        
        if (!metadata) {
            error_msg = "Plugin metadata (plugin.toml) not found for: " + path;
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
