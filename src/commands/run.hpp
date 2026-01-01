#pragma once

#include "build.hpp"
#include "../toml/toml.hpp"
#include "../../plugins/tracer/src/tracer_plugin.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace forma::commands {

struct RunOptions {
    std::string project_dir;
    std::string target;  // esp32, linux, windows, etc.
    bool verbose = false;
    bool flash = false;   // Flash after build (for embedded targets)
    bool monitor = false; // Start monitor after flash (for embedded targets)
};

int run_run_command(const RunOptions& opts) {
    auto& tracer = forma::tracer::get_tracer();
    
    if (opts.verbose) {
        tracer.set_level(forma::tracer::TraceLevel::Verbose);
    }
    
    tracer.info("Forma Run Command");
    tracer.info("=================\n");
    
    std::string project_dir = opts.project_dir.empty() ? "." : opts.project_dir;
    
    // Step 1 & 2: Use build command to compile and build
    BuildOptions build_opts;
    build_opts.project_dir = opts.project_dir;
    build_opts.target = opts.target;
    build_opts.verbose = opts.verbose;
    build_opts.flash = opts.flash;
    build_opts.monitor = opts.monitor;
    
    int build_result = run_build_command(build_opts);
    if (build_result != 0) {
        return build_result;
    }
    
    // Step 3: Run the application (only for native targets without flash/monitor)
    // For embedded targets with flash/monitor, the build plugin already handled execution
    if (!opts.flash && !opts.monitor) {
        tracer.begin_stage("Running application");
        
        // Read config to get build system and executable name
        auto config = read_project_config(project_dir, tracer);
        
        // Get executable name from project config
        std::string executable_name = "app";
        std::filesystem::path toml_path = std::filesystem::path(project_dir) / "project.toml";
        if (!std::filesystem::exists(toml_path)) {
            toml_path = std::filesystem::path(project_dir) / "forma.toml";
        }
        
        if (std::filesystem::exists(toml_path)) {
            std::ifstream file(toml_path);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string toml_content = buffer.str();
            auto doc = forma::toml::parse(toml_content);
            
            if (auto* project_table = doc.get_table("project")) {
                if (auto val = project_table->get_string("name")) {
                    executable_name = std::string(*val);
                }
            }
        }
        
        // Determine executable path based on build system
        std::filesystem::path executable_path;
        
        if (config.build_system == "cmake-generator") {
            // CMake builds to build/ directory
            executable_path = std::filesystem::path(project_dir) / "build" / executable_name;
        } else if (config.build_system == "esp32-lvgl") {
            // ESP32 builds are flashed to device, not run locally
            tracer.warning("ESP32 applications run on device, not locally");
            tracer.info("Use --flash --monitor to flash and monitor the device");
            tracer.end_stage();
            return 0;
        } else {
            // Default: assume build/ directory
            executable_path = std::filesystem::path(project_dir) / "build" / executable_name;
        }
        
        if (!std::filesystem::exists(executable_path)) {
            tracer.error(std::string("Executable not found: ") + executable_path.string());
            tracer.info("Build may have failed or executable name may be incorrect");
            tracer.end_stage();
            return 1;
        }
        
        tracer.info(std::string("Running: ") + executable_path.string());
        tracer.end_stage();
        
        // Execute the program
        std::string run_cmd = executable_path.string();
        int exit_code = system(run_cmd.c_str());
        
        if (exit_code != 0) {
            tracer.error(std::string("Application exited with code: ") + std::to_string(exit_code));
            return exit_code;
        }
        
        tracer.info("✓ Application completed successfully");
    }
    
    return 0;
}

} // namespace forma::commands
