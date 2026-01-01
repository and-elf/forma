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
#include "core/fs/i_file_system.hpp"
#include "core/host_context.hpp"
#include <random>
#include <chrono>

namespace forma {

// Plugin function pointers loaded from dynamic library
// Low-level C ABI function pointers
struct PluginFunctions {
    bool (*render)(const void* doc, const char* input_path, const char* output_path);
    int (*build)(const char* project_dir, const char* config_path, bool verbose, bool flash, bool monitor);  // For build plugins
    void (*register_plugin)(void* host);  // Optional
    uint64_t (*get_metadata_hash)();  // Required - returns hash of expected TOML
};

// High-level adapters that the host can call with an IFileSystem
using RendererAdapter = std::function<bool(const void* doc,
                                           const std::string& input_path,
                                           const std::string& output_path,
                                           forma::fs::IFileSystem& fs)>;

using BuildAdapter = std::function<int(const std::string& project_dir,
                                       const std::string& config_path,
                                       forma::fs::IFileSystem& fs,
                                       bool verbose, bool flash, bool monitor)>;

struct LoadedPlugin {
    void* handle = nullptr;
    PluginFunctions functions;
    std::string path;
    std::unique_ptr<PluginMetadata> metadata;  // Loaded from plugin.toml
    // High-level adapters generated at load time
    RendererAdapter renderer_adapter;
    BuildAdapter build_adapter;
    std::unique_ptr<forma::HostContext> host_context;
    
    ~LoadedPlugin() {
        if (handle) {
            dlclose(handle);
        }
    }
};

// Interface for plugin loader to allow mocking in tests
struct IPluginLoader {
    virtual ~IPluginLoader() = default;
    virtual bool load_plugin(const std::string& path, std::string& error_msg) = 0;
    virtual int load_plugins_from_directory(const std::string& dir_path, std::vector<std::string>& errors) = 0;
    virtual bool load_plugin_by_name(const std::string& plugin_name, std::string& error_msg) = 0;
    virtual void add_plugin_search_path(const std::string& dir_path) = 0;
    virtual void register_builtin_plugin(
        bool (*render_fn)(const void*, const char*, const char*),
        int (*build_fn)(const char*, const char*, bool, bool, bool),
        void (*register_fn)(void*),
        std::unique_ptr<PluginMetadata> metadata) = 0;
    virtual const std::vector<std::unique_ptr<LoadedPlugin>>& get_loaded_plugins() const = 0;
    virtual RendererAdapter get_renderer_adapter(const std::string& name) = 0;
    virtual BuildAdapter get_build_adapter(const std::string& name) = 0;
    virtual LoadedPlugin* find_plugin(const std::string& name) = 0;
    virtual void print_loaded_plugins(std::ostream& out = std::cout) const = 0;
};

class PluginLoader : public IPluginLoader {
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
        
        // Look for the forma_render function (optional - for renderer plugins)
        typedef bool (*RenderFunc)(const void*, const char*, const char*);
        RenderFunc render_fn = reinterpret_cast<RenderFunc>(dlsym(handle, "forma_render"));
        dlerror(); // Clear error (it's optional)
        
        // Look for the forma_build function (optional - for build plugins)
        typedef int (*BuildFunc)(const char*, const char*, bool, bool, bool);
        BuildFunc build_fn = reinterpret_cast<BuildFunc>(dlsym(handle, "forma_build"));
        dlerror(); // Clear error (it's optional)
        
        // Plugin must have at least one function (render or build)
        if (!render_fn && !build_fn) {
            error_msg = "Plugin must provide at least one of: forma_render, forma_build";
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
        
        const char* dlsym_error = dlerror();
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
        loaded->functions.build = build_fn;
        loaded->functions.register_plugin = register_fn;
        loaded->functions.get_metadata_hash = hash_fn;
        loaded->path = path;
        loaded->metadata = std::move(metadata);

        // Create adapters that bridge IFileSystem <-> plugin C ABI using temp files
        // Helper to make a unique temp path
        auto make_temp = [](const std::string& suffix) {
            auto tdir = std::filesystem::temp_directory_path();
            auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::mt19937_64 rng(static_cast<unsigned long long>(now ^ reinterpret_cast<uint64_t>(std::malloc(1))));
            std::uniform_int_distribution<uint64_t> dist;
            auto rnd = dist(rng);
            return (tdir / ("forma_plugin_" + std::to_string(now) + "_" + std::to_string(rnd) + suffix)).string();
        };

        // Wrap render fn
        if (render_fn) {
            loaded->renderer_adapter = [render_fn, make_temp](const void* doc, const std::string& input_path, const std::string& output_path, forma::fs::IFileSystem& fs) -> bool {
                try {
                    // Create temp input file from fs
                    std::string tmp_in = make_temp(".in");
                    std::ofstream of(tmp_in, std::ios::binary);
                    of << fs.read_file(input_path);
                    of.close();

                    // Temp output file
                    std::string tmp_out = make_temp(".out");

                    bool ok = render_fn(doc, tmp_in.c_str(), tmp_out.c_str());

                    if (ok) {
                        // Read tmp_out and write back into fs at output_path
                        std::ifstream inf(tmp_out, std::ios::binary);
                        std::string out_contents((std::istreambuf_iterator<char>(inf)), {});
                        inf.close();
                        fs.write_file(output_path, out_contents);
                    }

                    // Cleanup
                    std::error_code ec;
                    std::filesystem::remove(tmp_in, ec);
                    std::filesystem::remove(tmp_out, ec);
                    return ok;
                } catch (...) {
                    return false;
                }
            };
        }

        // Wrap build fn
        if (build_fn) {
            loaded->build_adapter = [build_fn, make_temp, &loaded](const std::string& project_dir, const std::string& config_path, forma::fs::IFileSystem& fs, bool verbose, bool flash, bool monitor) -> int {
                try {
                    // Create a temp project directory on disk
                    std::filesystem::path tmp_proj = std::filesystem::temp_directory_path() / ("forma_proj_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
                    std::filesystem::create_directories(tmp_proj);

                    // If config_path exists in fs, write it to a temp file
                    std::string tmp_config = (tmp_proj / "forma_plugin_config.toml").string();
                    try {
                        auto cfg = fs.read_file(config_path);
                        std::ofstream cof(tmp_config, std::ios::binary);
                        cof << cfg;
                        cof.close();
                    } catch (...) {
                        // ignore if not present
                    }

                    int rc = build_fn(tmp_proj.string().c_str(), tmp_config.c_str(), verbose, flash, monitor);

                    // If plugin has an associated HostContext filesystem, sync outputs back into it
                    if (loaded->host_context && loaded->host_context->filesystem) {
                        auto& target_fs = *loaded->host_context->filesystem;

                        // Recursively copy files from tmp_proj into target_fs under project_dir
                        for (auto& p : std::filesystem::recursive_directory_iterator(tmp_proj)) {
                            if (p.is_regular_file()) {
                                auto rel = std::filesystem::relative(p.path(), tmp_proj).string();
                                std::string dest = project_dir + "/" + rel;
                                // Read file content
                                std::ifstream inf(p.path(), std::ios::binary);
                                std::string content((std::istreambuf_iterator<char>(inf)), {});
                                inf.close();
                                // Ensure directory exists in target fs
                                auto parent = std::filesystem::path(dest).parent_path().string();
                                target_fs.create_dirs(parent);
                                target_fs.write_file(dest, content);
                            }
                        }
                    }

                    // Cleanup temp project directory
                    std::error_code ec;
                    std::filesystem::remove_all(tmp_proj, ec);
                    return rc;
                } catch (...) {
                    return -1;
                }
            };
        }

        // Save loaded plugin first
        loaded_plugins.push_back(std::move(loaded));

        // Call registration function if available; create and attach HostContext to plugin
        if (register_fn) {
            auto ctx = std::make_unique<HostContext>(std::make_unique<forma::fs::RealFileSystem>(), &forma::tracer::get_tracer());
            register_fn(static_cast<void*>(ctx.get()));
            loaded->host_context = std::move(ctx);
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
        int (*build_fn)(const char*, const char*, bool, bool, bool),
        void (*register_fn)(void*),
        std::unique_ptr<PluginMetadata> metadata) {
        
        if (!render_fn && !build_fn) {
            std::cerr << "Warning: Built-in plugin missing render/build function\n";
            return;
        }
        if (!metadata) {
            std::cerr << "Warning: Built-in plugin missing metadata\n";
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
        loaded->functions.build = build_fn;
        loaded->functions.register_plugin = register_fn;
        loaded->path = "builtin:" + metadata->name;
        loaded->metadata = std::move(metadata);
        // For builtins: create adapters and call register if present
        if (loaded->functions.render) {
            auto rf = loaded->functions.render;
            loaded->renderer_adapter = [rf](const void* doc, const std::string& in, const std::string& out, forma::fs::IFileSystem& fs) -> bool {
                try {
                    // Write input to temp file
                    std::string tmp_in = std::filesystem::temp_directory_path() / ("forma_builtin_in_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
                    std::ofstream of(tmp_in, std::ios::binary);
                    of << fs.read_file(in);
                    of.close();

                    std::string tmp_out = tmp_in + ".out";
                    bool ok = rf(doc, tmp_in.c_str(), tmp_out.c_str());
                    if (ok) {
                        std::ifstream inf(tmp_out, std::ios::binary);
                        std::string out_contents((std::istreambuf_iterator<char>(inf)), {});
                        inf.close();
                        fs.write_file(out, out_contents);
                    }
                    std::error_code ec;
                    std::filesystem::remove(tmp_in, ec);
                    std::filesystem::remove(tmp_out, ec);
                    return ok;
                } catch (...) { return false; }
            };
        }

        // For builtins: create build adapter if provided
        if (loaded->functions.build) {
            auto bf = loaded->functions.build;
            loaded->build_adapter = [bf](const std::string& /*project_dir*/, const std::string& /*config_path*/, forma::fs::IFileSystem& /*fs*/, bool verbose, bool flash, bool monitor) -> int {
                try {
                    std::filesystem::path tmp_proj = std::filesystem::temp_directory_path() / ("forma_builtin_proj_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
                    std::filesystem::create_directories(tmp_proj);
                    std::string tmp_config = (tmp_proj / "forma_plugin_config.toml").string();
                    int rc = bf(tmp_proj.string().c_str(), tmp_config.c_str(), verbose, flash, monitor);
                    // No automatic sync here: the loader's caller may handle HostContext sync similarly to dynamic plugins
                    std::error_code ec;
                    std::filesystem::remove_all(tmp_proj, ec);
                    return rc;
                } catch (...) {
                    return -1;
                }
            };
        }

        // Call register function for builtin if present
        if (loaded->functions.register_plugin) {
            auto ctx = std::make_unique<HostContext>(std::make_unique<forma::fs::RealFileSystem>(), &forma::tracer::get_tracer());
            loaded->functions.register_plugin(static_cast<void*>(ctx.get()));
            loaded->host_context = std::move(ctx);
        }

        loaded_plugins.push_back(std::move(loaded));
    }
    
    // Get all loaded plugins
    const std::vector<std::unique_ptr<LoadedPlugin>>& get_loaded_plugins() const {
        return loaded_plugins;
    }

    // Get adapters for a plugin by name (returns null if not available)
    RendererAdapter get_renderer_adapter(const std::string& name) {
        auto p = find_plugin(name);
        if (!p) return RendererAdapter();
        return p->renderer_adapter;
    }

    BuildAdapter get_build_adapter(const std::string& name) {
        auto p = find_plugin(name);
        if (!p) return BuildAdapter();
        return p->build_adapter;
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
