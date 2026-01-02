#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <fstream>
#include "core/toml_io.hpp"

namespace forma::config {

// Project configuration loaded from forma.toml
struct ProjectConfig {
    std::string project_root;
    std::string toolchain_dir;    // Where to store downloaded toolchains
    std::string cache_dir;         // Where to store cached downloads
    std::string build_dir;         // Build output directory
    
    // Defaults
    ProjectConfig() 
        : toolchain_dir(".forma/toolchains")
        , cache_dir(".forma/cache")
        , build_dir("build")
    {}
};

// Find the project root by looking for forma.toml or project.toml
inline std::optional<std::string> find_project_root(const std::string& start_path = ".") {
    namespace fs = std::filesystem;
    
    fs::path current = fs::absolute(start_path);
    
    // Search up to 10 levels
    for (int i = 0; i < 10; ++i) {
        if (fs::exists(current / "forma.toml") || fs::exists(current / "project.toml")) {
            return current.string();
        }
        
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        
        current = current.parent_path();
    }
    
    return std::nullopt;
}

// Load project configuration from forma.toml
inline ProjectConfig load_project_config(const std::string& project_root = ".") {
    namespace fs = std::filesystem;
    
    ProjectConfig config;
    
    // Find actual project root if not provided
    std::string root = project_root;
    if (root == ".") {
        auto found = find_project_root(root);
        if (found) {
            root = *found;
        }
    }
    
    config.project_root = root;
    
    // Try to load forma.toml or project.toml
    fs::path toml_path = fs::path(root) / "forma.toml";
    if (!fs::exists(toml_path)) {
        toml_path = fs::path(root) / "project.toml";
    }
    
    if (!fs::exists(toml_path)) {
        // No config file, use defaults
        return config;
    }
    
    // Read TOML file via RealFileSystem + core helper
    forma::fs::RealFileSystem realfs;
    auto doc_opt = forma::core::parse_toml_from_fs(realfs, toml_path.string());
    if (!doc_opt) return config;
    auto& doc = *doc_opt;

    // Parse [toolchains] section
    auto toolchains_table = doc.get_table("toolchains");
    if (toolchains_table) {
        auto dir = toolchains_table->get_string("directory");
        if (dir) {
            config.toolchain_dir = *dir;
        }
    }
    
    // Parse [cache] section
    auto cache_table = doc.get_table("cache");
    if (cache_table) {
        auto dir = cache_table->get_string("directory");
        if (dir) {
            config.cache_dir = *dir;
        }
    }
    
    // Parse [build] section
    auto build_table = doc.get_table("build");
    if (build_table) {
        auto dir = build_table->get_string("directory");
        if (dir) {
            config.build_dir = *dir;
        }
    }
    
    return config;
}

// Get absolute path for a config directory (relative to project root)
inline std::string get_absolute_path(const ProjectConfig& config, const std::string& relative_path) {
    namespace fs = std::filesystem;
    
    fs::path abs_path = fs::path(config.project_root) / relative_path;
    return abs_path.string();
}

// Get toolchain directory (absolute path)
inline std::string get_toolchain_dir(const ProjectConfig& config) {
    return get_absolute_path(config, config.toolchain_dir);
}

// Get cache directory (absolute path)
inline std::string get_cache_dir(const ProjectConfig& config) {
    return get_absolute_path(config, config.cache_dir);
}

// Get build directory (absolute path)
inline std::string get_build_dir(const ProjectConfig& config) {
    return get_absolute_path(config, config.build_dir);
}

// Ensure a directory exists
inline bool ensure_directory(const std::string& path) {
    namespace fs = std::filesystem;
    
    try {
        fs::create_directories(path);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace forma::config
