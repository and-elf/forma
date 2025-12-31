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

struct DeployOptions {
    std::string project_dir;
    std::vector<std::string> deploy_systems;  // Multiple deployment systems
    std::vector<std::string> architectures;   // Target architectures
    bool verbose = false;
    
    // Package metadata overrides (optional, override TOML values)
    std::string package_name;
    std::string package_version;
    std::string maintainer;
    std::string description;
};

struct ProjectInfo {
    std::string deploy_system;
    bool is_plugin = false;
    std::string plugin_name;
};

// Read project.toml/forma.toml/plugin.toml and extract deployment configuration
ProjectInfo read_deploy_config(const std::string& project_dir, forma::tracer::TracerPlugin& tracer) {
    std::filesystem::path project_path(project_dir);
    std::filesystem::path toml_path = project_path / "project.toml";
    
    // Try forma.toml if project.toml doesn't exist
    if (!std::filesystem::exists(toml_path)) {
        toml_path = project_path / "forma.toml";
    }
    
    // Try plugin.toml if forma.toml doesn't exist
    if (!std::filesystem::exists(toml_path)) {
        toml_path = project_path / "plugin.toml";
    }
    
    if (!std::filesystem::exists(toml_path)) {
        tracer.error("No project.toml, forma.toml, or plugin.toml found in project directory");
        return {};
    }
    
    tracer.verbose(std::string("Reading project configuration: ") + toml_path.string());
    
    // Read TOML file
    std::ifstream file(toml_path);
    if (!file.is_open()) {
        tracer.error(std::string("Failed to open: ") + toml_path.string());
        return {};
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string toml_content = buffer.str();
    
    // Parse TOML
    auto doc = forma::toml::parse(toml_content);
    
    ProjectInfo info;
    
    // Check if this is a plugin
    auto* plugin_table = doc.get_table("plugin");
    if (plugin_table) {
        info.is_plugin = true;
        auto name = plugin_table->get_string("name");
        if (name) {
            info.plugin_name = std::string(name.value());
            tracer.verbose(std::string("Detected plugin: ") + info.plugin_name);
        }
    }
    
    // Get [deploy] table
    auto* deploy_table = doc.get_table("deploy");
    if (!deploy_table) {
        tracer.error("No [deploy] section found in configuration");
        tracer.info("Add a [deploy] section with: system = \"deb\" (or rpm, etc.)");
        return {};
    }
    
    // Get system value
    auto system = deploy_table->get_string("system");
    if (!system) {
        tracer.error("No 'system' key found in [deploy] section");
        return {};
    }
    
    info.deploy_system = std::string(system.value());
    return info;
}

// Call the debian package builder plugin
int call_deb_deploy_plugin(const std::string& project_dir, forma::tracer::TracerPlugin& tracer) {
    // Try multiple possible plugin locations
    std::vector<std::filesystem::path> plugin_paths = {
        std::filesystem::path(project_dir) / "build" / "plugins" / "forma-deb-deploy.so",
        "../deb-deploy/build/forma-deb-deploy.so",          // From sibling plugin dir
        "../../plugins/deb-deploy/build/forma-deb-deploy.so", // From plugin subdir
        "../plugins/deb-deploy/build/forma-deb-deploy.so",  // Relative to test dir
        "plugins/deb-deploy/build/forma-deb-deploy.so",     // From forma root
        std::filesystem::path("build") / "plugins" / "forma-deb-deploy.so",
        "/usr/local/lib/forma/plugins/forma-deb-deploy.so"
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
        tracer.error("deb-deploy plugin not found. Build it first with:");
        tracer.info("  cd plugins/deb-deploy && cmake -B build && cmake --build build");
        tracer.info("Or install it system-wide.");
        tracer.info("Searched paths:");
        for (const auto& path : plugin_paths) {
            tracer.info(std::string("  - ") + path.string());
        }
        return 1;
    }
    
    tracer.verbose(std::string("Loading deb-deploy plugin: ") + plugin_path.string());
    
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

int run_deploy_command(const DeployOptions& opts) {
    auto& tracer = forma::tracer::get_tracer();
    
    if (opts.verbose) {
        tracer.set_level(forma::tracer::TraceLevel::Verbose);
    }
    
    tracer.info("Forma Deploy Command");
    tracer.info("===================\n");
    
    std::string project_dir = opts.project_dir.empty() ? "." : opts.project_dir;
    
    // Determine deploy systems to use
    std::vector<std::string> deploy_systems = opts.deploy_systems;
    bool is_plugin = false;
    std::string plugin_name;
    
    // If no deploy systems specified on command line, read from config
    if (deploy_systems.empty()) {
        auto info = read_deploy_config(project_dir, tracer);
        if (info.deploy_system.empty()) {
            return 1;
        }
        deploy_systems.push_back(info.deploy_system);
        is_plugin = info.is_plugin;
        plugin_name = info.plugin_name;
        
        if (is_plugin) {
            tracer.info(std::string("Plugin: ") + plugin_name);
        }
        tracer.info(std::string("Deploy system: ") + info.deploy_system + " (from config)");
    } else {
        // Check if it's a plugin even when deploy systems are specified
        auto info = read_deploy_config(project_dir, tracer);
        is_plugin = info.is_plugin;
        plugin_name = info.plugin_name;
        
        if (is_plugin) {
            tracer.info(std::string("Plugin: ") + plugin_name);
        }
        tracer.info(std::string("Deploy systems: ") + std::to_string(deploy_systems.size()) + " specified");
        if (opts.verbose) {
            for (const auto& sys : deploy_systems) {
                tracer.verbose(std::string("  - ") + sys);
            }
        }
    }
    
    // Determine architectures
    std::vector<std::string> architectures = opts.architectures;
    if (architectures.empty()) {
        // Default to current system architecture
        architectures.push_back("amd64");  // TODO: Detect from system
        tracer.verbose("Using default architecture: amd64");
    } else {
        tracer.info(std::string("Architectures: ") + std::to_string(architectures.size()) + " specified");
        if (opts.verbose) {
            for (const auto& arch : architectures) {
                tracer.verbose(std::string("  - ") + arch);
            }
        }
    }
    
    // Build all combinations
    int total_builds = deploy_systems.size() * architectures.size();
    int successful = 0;
    int failed = 0;
    
    tracer.info(std::string("\nBuilding ") + std::to_string(total_builds) + " package(s)...\n");
    
    for (const auto& deploy_system : deploy_systems) {
        for (const auto& arch : architectures) {
            tracer.info(std::string("Building: ") + deploy_system + " (" + arch + ")");
            
            // Currently only deb is supported
            if (deploy_system == "deb" || deploy_system == "debian") {
                int result = call_deb_deploy_plugin(project_dir, tracer);
                if (result == 0) {
                    successful++;
                } else {
                    failed++;
                    tracer.error(std::string("Failed to build: ") + deploy_system + " (" + arch + ")");
                }
            } else {
                tracer.error(std::string("Unsupported deploy system: ") + deploy_system);
                tracer.info("Supported systems: deb, debian");
                failed++;
            }
        }
    }
    
    // Summary
    tracer.info(std::string("\n==================="));
    tracer.info(std::string("Deployment Summary"));
    tracer.info(std::string("==================="));
    tracer.info(std::string("Total: ") + std::to_string(total_builds));
    tracer.info(std::string("✓ Successful: ") + std::to_string(successful));
    if (failed > 0) {
        tracer.error(std::string("✗ Failed: ") + std::to_string(failed));
        return 1;
    }
    
    return 0;
}

} // namespace forma::commands
