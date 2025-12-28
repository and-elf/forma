#pragma once

#include <dlfcn.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <cstdint>

namespace forma {

// Plugin descriptor structure (minimal version for loading)
struct PluginCapabilities {
    bool supports_renderer;
    bool supports_theme;
    bool supports_audio;
    bool supports_build;
    
    // Renderer callback: render(doc, input_path, output_path) -> success
    bool (*render)(const void* doc, const char* input_path, const char* output_path);
};

struct FormaPluginDescriptor {
    uint32_t api_version;
    const char* name;
    const char* version;
    PluginCapabilities capabilities;
    void* register_plugin;  // Function pointer
};

// ============================================================================
// Dynamic Plugin Loader
// ============================================================================

struct LoadedPlugin {
    void* handle = nullptr;
    FormaPluginDescriptor* descriptor = nullptr;
    std::string path;
    
    ~LoadedPlugin() {
        if (handle) {
            dlclose(handle);
        }
    }
};

class PluginLoader {
private:
    std::vector<LoadedPlugin*> loaded_plugins;
    
public:
    ~PluginLoader() {
        for (auto* plugin : loaded_plugins) {
            delete plugin;
        }
    }
    
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
        
        // Look for the plugin initialization function
        typedef FormaPluginDescriptor* (*GetPluginDescriptorFunc)();
        GetPluginDescriptorFunc get_descriptor = 
            reinterpret_cast<GetPluginDescriptorFunc>(
                dlsym(handle, "forma_get_plugin_descriptor")
            );
        
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            error_msg = std::string("Cannot find forma_get_plugin_descriptor: ") + dlsym_error;
            dlclose(handle);
            return false;
        }
        
        // Get the plugin descriptor
        FormaPluginDescriptor* descriptor = get_descriptor();
        if (!descriptor) {
            error_msg = "Plugin descriptor is null";
            dlclose(handle);
            return false;
        }
        
        // Validate API version
        if (descriptor->api_version != 1) {
            error_msg = std::string("Incompatible API version: expected 1, got ") + 
                       std::to_string(descriptor->api_version);
            dlclose(handle);
            return false;
        }
        
        // Store the loaded plugin
        auto* loaded = new LoadedPlugin{handle, descriptor, path};
        loaded_plugins.push_back(loaded);
        
        // Register the plugin if it has a registration function
        if (descriptor->register_plugin) {
            // TODO: Create FormaHost instance and pass it
            // For now, just acknowledge the plugin is loaded
        }
        
        return true;
    }
    
    // Get all loaded plugins
    const std::vector<LoadedPlugin*>& get_loaded_plugins() const {
        return loaded_plugins;
    }
    
    // Get plugin by name
    LoadedPlugin* find_plugin(const std::string& name) {
        for (auto* plugin : loaded_plugins) {
            if (plugin->descriptor && 
                std::string(plugin->descriptor->name) == name) {
                return plugin;
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
        for (const auto* plugin : loaded_plugins) {
            if (plugin->descriptor) {
                out << "  - " << plugin->descriptor->name 
                    << " v" << plugin->descriptor->version;
                
                // Show capabilities
                std::vector<std::string> caps;
                if (plugin->descriptor->capabilities.supports_renderer) caps.push_back("renderer");
                if (plugin->descriptor->capabilities.supports_theme) caps.push_back("theme");
                if (plugin->descriptor->capabilities.supports_audio) caps.push_back("audio");
                if (plugin->descriptor->capabilities.supports_build) caps.push_back("build");
                
                if (!caps.empty()) {
                    out << " [";
                    for (size_t i = 0; i < caps.size(); ++i) {
                        out << caps[i];
                        if (i < caps.size() - 1) out << ", ";
                    }
                    out << "]";
                }
                
                out << "\n    Path: " << plugin->path << "\n";
            }
        }
    }
};

} // namespace forma
