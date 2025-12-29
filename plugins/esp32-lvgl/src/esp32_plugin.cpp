#include "esp32_build_system.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

extern "C" {

const char* get_plugin_name() {
    return "esp32";
}

const char* get_plugin_version() {
    return "0.1.0";
}

const char* get_plugin_description() {
    return "ESP32 build system plugin - sets up ESP-IDF and toolchain";
}

const char* get_plugin_type() {
    return "build-system";
}

// Setup ESP32 project with ESP-IDF
bool setup_esp32_project(const char* project_dir, const char* config_file) {
    if (!project_dir) {
        std::cerr << "Error: Invalid project directory\n";
        return false;
    }
    
    std::cout << "Setting up ESP32 project in: " << project_dir << "\n";
    
    forma::esp32::ESP32BuildSystem builder(project_dir);
    forma::esp32::ESP32Config config;
    
    // Load configuration from TOML if provided
    if (config_file && std::filesystem::exists(config_file)) {
        std::cout << "Loading configuration from: " << config_file << "\n";
        
        std::ifstream file(config_file);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        
        forma::esp32::parse_esp32_config(buffer.str(), config);
    }
    
    builder.set_config(config);
    
    if (!builder.setup_project()) {
        std::cerr << "Error: Failed to setup ESP32 project\n";
        return false;
    }
    
    std::cout << "\nESP32 project configured successfully!\n";
    std::cout << "\nConfiguration:\n";
    std::cout << "  IDF Version: " << config.idf_version << "\n";
    std::cout << "  Target: " << config.target << "\n";
    if (!config.idf_path.empty()) {
        std::cout << "  IDF Path: " << config.idf_path << "\n";
    }
    
    return true;
}

} // extern "C"
