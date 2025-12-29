#pragma once

#include "../toml/toml.hpp"
#include "../plugin_loader.hpp"
#include "../../plugins/tracer/src/tracer_plugin.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <dlfcn.h>

namespace forma::commands {

struct ReleaseOptions {
    std::string project_dir;
    std::string release_system;  // Override from command line
    bool verbose = false;
};

// Read project.toml and extract release configuration
std::string read_release_config(const std::string& project_dir, forma::tracer::TracerPlugin& tracer) {
    std::filesystem::path project_path(project_dir);
    std::filesystem::path toml_path = project_path / "project.toml";
    
    // Try forma.toml if project.toml doesn't exist
    if (!std::filesystem::exists(toml_path)) {
        toml_path = project_path / "forma.toml";
    }
    
    if (!std::filesystem::exists(toml_path)) {
        tracer.error("No project.toml or forma.toml found in project directory");
        return "";
    }
    
    tracer.verbose(std::string("Reading project configuration: ") + toml_path.string());
    
    // Read TOML file
    std::ifstream file(toml_path);
    if (!file.is_open()) {
        tracer.error(std::string("Failed to open: ") + toml_path.string());
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string toml_content = buffer.str();
    
    // Parse TOML
    auto doc = forma::toml::parse(toml_content);
    
    // Get [release] table
    auto* release_table = doc.get_table("release");
    if (!release_table) {
        tracer.error("No [release] section found in project configuration");
        return "";
    }
    
    // Get system value
    auto system = release_table->get_string("system");
    if (!system) {
        tracer.error("No 'system' key found in [release] section");
        return "";
    }
    
    return std::string(system.value());
}

// Call the debian package builder plugin
int call_deb_release_plugin(const std::string& project_dir, forma::tracer::TracerPlugin& tracer) {
    // Try multiple possible plugin locations
    std::vector<std::filesystem::path> plugin_paths = {
        std::filesystem::path(project_dir) / "build" / "plugins" / "forma-deb-release.so",
        "../plugins/deb-release/build/forma-deb-release.so",  // Relative to test dir
        "plugins/deb-release/build/forma-deb-release.so",     // From forma root
        std::filesystem::path("build") / "plugins" / "forma-deb-release.so",
        "/usr/local/lib/forma/plugins/forma-deb-release.so"
    };
    
    std::filesystem::path plugin_path;
    bool found = false;
    
    for (const auto& path : plugin_paths) {
        tracer.verbose(std::string("  Checking: ") + path.string());
        if (std::filesystem::exists(path)) {
            plugin_path = path;
            found = true;
            tracer.verbose(std::string("  Found at: ") + path.string());
            break;
        }
    }
    
    if (!found) {
        tracer.error("deb-release plugin not found. Build it first with:");
        tracer.info("  cd plugins/deb-release && cmake -B build && cmake --build build");
        tracer.info("Or install it system-wide.");
        tracer.info("Searched paths:");
        for (const auto& path : plugin_paths) {
            tracer.info(std::string("  - ") + path.string());
        }
        return 1;
    }
    
    tracer.verbose(std::string("Loading deb-release plugin: ") + plugin_path.string());
    
    // Load the plugin dynamically
    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
    if (!handle) {
        tracer.error(std::string("Failed to load plugin: ") + dlerror());
        return 1;
    }
    
    // Find the create_debian_package function
    typedef bool (*CreateDebianPackageFn)(const char*, const char*, const char*);
    auto create_debian_package = (CreateDebianPackageFn)dlsym(handle, "create_debian_package");
    
    if (!create_debian_package) {
        tracer.error(std::string("Plugin does not export create_debian_package function: ") + dlerror());
        dlclose(handle);
        return 1;
    }
    
    // Look for package.cfg in project directory
    std::filesystem::path config_path = std::filesystem::path(project_dir) / "package.cfg";
    if (!std::filesystem::exists(config_path)) {
        tracer.error("No package.cfg found in project directory. Create one with package metadata.");
        tracer.info("Example package.cfg:");
        tracer.info("  name=myapp");
        tracer.info("  version=1.0.0");
        tracer.info("  architecture=amd64");
        tracer.info("  maintainer=Your Name <you@example.com>");
        tracer.info("  description=My awesome application");
        dlclose(handle);
        return 1;
    }
    
    // Setup build and source directories
    std::filesystem::path build_dir = std::filesystem::path(project_dir) / "build" / "debian-package";
    std::filesystem::path source_dir = std::filesystem::path(project_dir) / "build";  // Where compiled binaries are
    
    // Create build directory if it doesn't exist
    std::filesystem::create_directories(build_dir);
    
    tracer.info(std::string("Building Debian package from: ") + config_path.string());
    tracer.verbose(std::string("  Build dir: ") + build_dir.string());
    tracer.verbose(std::string("  Source dir: ") + source_dir.string());
    
    // Call the plugin
    bool result = create_debian_package(build_dir.c_str(), source_dir.c_str(), config_path.c_str());
    
    dlclose(handle);
    
    if (result) {
        tracer.info("✓ Debian package created successfully");
        return 0;
    } else {
        tracer.error("Failed to create Debian package");
        return 1;
    }
}

int run_release_command(const ReleaseOptions& opts) {
    auto& tracer = forma::tracer::get_tracer();
    
    if (opts.verbose) {
        tracer.set_level(forma::tracer::TraceLevel::Verbose);
    }
    
    tracer.info("Forma Release Command");
    tracer.info("=====================\n");
    
    std::string project_dir = opts.project_dir.empty() ? "." : opts.project_dir;
    std::string release_system = opts.release_system;
    
    // If no release system specified on command line, read from project.toml
    if (release_system.empty()) {
        release_system = read_release_config(project_dir, tracer);
        if (release_system.empty()) {
            return 1;
        }
        tracer.info(std::string("Release system: ") + release_system + " (from project.toml)");
    } else {
        tracer.info(std::string("Release system: ") + release_system + " (from --release-system)");
    }
    
    // Currently only deb is supported
    if (release_system == "deb" || release_system == "debian") {
        return call_deb_release_plugin(project_dir, tracer);
    } else {
        tracer.error(std::string("Unsupported release system: ") + release_system);
        tracer.info("Supported systems: deb, debian");
        return 1;
    }
}

} // namespace forma::commands
