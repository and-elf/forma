#pragma once

#include "../toml/toml.hpp"
#include "../plugin_loader.hpp"
#include "../../plugins/tracer/src/tracer_plugin.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace forma::commands {

struct BuildOptions {
    std::string project_dir;
    std::string target;  // esp32, linux, windows, etc.
    bool verbose = false;
    bool flash = false;   // Flash after build (for embedded targets)
    bool monitor = false; // Start monitor after flash
};

// Read project.toml and extract build configuration
std::string read_build_config(const std::string& project_dir, forma::tracer::TracerPlugin& tracer, std::string& target) {
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
    
    // Get [build] table
    auto* build_table = doc.get_table("build");
    if (!build_table) {
        tracer.error("No [build] section found in project configuration");
        return "";
    }
    
    // Get target value
    auto target_val = build_table->get_string("target");
    if (target_val) {
        target = std::string(target_val.value());
    }
    
    // Get system value (build system)
    auto system = build_table->get_string("system");
    if (!system) {
        return "cmake"; // Default
    }
    
    return std::string(system.value());
}

int run_build_command(const BuildOptions& opts) {
    auto& tracer = forma::tracer::get_tracer();
    
    if (opts.verbose) {
        tracer.set_level(forma::tracer::TraceLevel::Verbose);
    }
    
    tracer.info("Forma Build Command");
    tracer.info("===================\n");
    
    std::string project_dir = opts.project_dir.empty() ? "." : opts.project_dir;
    std::string target = opts.target;
    std::string build_system;
    
    // If no target specified on command line, read from project.toml
    if (target.empty()) {
        build_system = read_build_config(project_dir, tracer, target);
        if (target.empty()) {
            tracer.info("No target specified, using default cmake build");
            target = "native";
        } else {
            tracer.info(std::string("Target: ") + target + " (from project.toml)");
        }
    } else {
        tracer.info(std::string("Target: ") + target + " (from --target)");
    }
    
    // Handle ESP32 targets
    if (target.find("esp32") == 0) {
        tracer.info("Building for ESP32 target...");
        
        // Check if IDF_PATH is set
        const char* idf_path = std::getenv("IDF_PATH");
        if (!idf_path) {
            tracer.error("IDF_PATH not set. Please run:");
            tracer.info("  source ~/esp/esp-idf/export.sh");
            return 1;
        }
        
        tracer.verbose(std::string("Using ESP-IDF at: ") + idf_path);
        
        // Build with idf.py
        std::ostringstream build_cmd;
        build_cmd << "cd " << project_dir << " && idf.py build";
        
        tracer.info("Executing: idf.py build");
        int result = system(build_cmd.str().c_str());
        
        if (result != 0) {
            tracer.error("Build failed");
            return 1;
        }
        
        tracer.info("✓ Build successful");
        
        // Flash if requested
        if (opts.flash) {
            tracer.info("Flashing to device...");
            std::ostringstream flash_cmd;
            flash_cmd << "cd " << project_dir << " && idf.py flash";
            
            result = system(flash_cmd.str().c_str());
            if (result != 0) {
                tracer.error("Flash failed");
                return 1;
            }
            
            tracer.info("✓ Flash successful");
        }
        
        // Start monitor if requested
        if (opts.monitor) {
            tracer.info("Starting monitor...");
            std::ostringstream monitor_cmd;
            monitor_cmd << "cd " << project_dir << " && idf.py monitor";
            
            result = system(monitor_cmd.str().c_str());
            return result;
        }
        
        return 0;
    }
    
    // Default cmake build for native targets
    tracer.info("Building with CMake...");
    
    std::ostringstream build_cmd;
    build_cmd << "cd " << project_dir << " && cmake -B build && cmake --build build";
    
    tracer.info("Executing: cmake build");
    int result = system(build_cmd.str().c_str());
    
    if (result != 0) {
        tracer.error("Build failed");
        return 1;
    }
    
    tracer.info("✓ Build successful");
    return 0;
}

} // namespace forma::commands
