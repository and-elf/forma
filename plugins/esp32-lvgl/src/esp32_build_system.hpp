#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <sys/stat.h>

namespace forma::esp32 {

struct ESP32Config {
    std::string idf_version = "v5.1";
    std::string target = "esp32";           // esp32, esp32s2, esp32s3, esp32c3, esp32c6
    std::string idf_path;
    std::string toolchain_path;
    bool auto_install = true;
    bool download_toolchain = true;
};

class ESP32BuildSystem {
private:
    ESP32Config config;
    std::string project_path;
    
    bool execute_command(const std::string& cmd) {
        std::cout << "Executing: " << cmd << "\n";
        int result = system(cmd.c_str());
        return result == 0;
    }
    
public:
    ESP32BuildSystem(const std::string& project_path) 
        : project_path(project_path) {}
    
    void set_config(const ESP32Config& cfg) {
        config = cfg;
    }
    
    bool check_idf_installation() {
        // Check if IDF_PATH is set
        const char* idf_path_env = std::getenv("IDF_PATH");
        if (idf_path_env) {
            config.idf_path = idf_path_env;
            std::cout << "Found ESP-IDF at: " << config.idf_path << "\n";
            return true;
        }
        
        // Check common installation locations
        std::vector<std::string> common_paths = {
            std::string(std::getenv("HOME")) + "/esp/esp-idf",
            "/opt/esp-idf",
            std::string(std::getenv("HOME")) + "/.espressif/esp-idf"
        };
        
        for (const auto& path : common_paths) {
            if (std::filesystem::exists(path + "/export.sh")) {
                config.idf_path = path;
                std::cout << "Found ESP-IDF at: " << path << "\n";
                return true;
            }
        }
        
        return false;
    }
    
    bool install_esp_idf() {
        if (!config.auto_install) {
            std::cerr << "Auto-install disabled. Please install ESP-IDF manually.\n";
            return false;
        }
        
        std::string install_path = std::string(std::getenv("HOME")) + "/esp/esp-idf";
        std::cout << "Installing ESP-IDF " << config.idf_version << " to " << install_path << "\n";
        
        // Create esp directory
        std::filesystem::create_directories(std::string(std::getenv("HOME")) + "/esp");
        
        // Clone ESP-IDF
        std::ostringstream clone_cmd;
        clone_cmd << "cd " << std::getenv("HOME") << "/esp && "
                  << "git clone --recursive --branch " << config.idf_version 
                  << " https://github.com/espressif/esp-idf.git";
        
        if (!execute_command(clone_cmd.str())) {
            std::cerr << "Failed to clone ESP-IDF\n";
            return false;
        }
        
        // Install tools
        std::cout << "Installing ESP-IDF tools...\n";
        std::string install_cmd = install_path + "/install.sh " + config.target;
        if (!execute_command(install_cmd)) {
            std::cerr << "Failed to install ESP-IDF tools\n";
            return false;
        }
        
        config.idf_path = install_path;
        std::cout << "ESP-IDF installed successfully!\n";
        std::cout << "Set environment: source " << install_path << "/export.sh\n";
        
        return true;
    }
    
    bool setup_project() {
        std::cout << "Setting up ESP-IDF project at: " << project_path << "\n";
        
        // Ensure IDF is available
        if (!check_idf_installation()) {
            std::cout << "ESP-IDF not found. Installing...\n";
            if (!install_esp_idf()) {
                return false;
            }
        }
        
        // Create project structure
        std::filesystem::create_directories(project_path + "/main");
        std::filesystem::create_directories(project_path + "/components");
        
        // Generate CMakeLists.txt (root)
        generate_root_cmake();
        
        // Generate main/CMakeLists.txt
        generate_main_cmake();
        
        // Generate main/main.c
        generate_main_c();
        
        // Generate sdkconfig.defaults
        generate_sdkconfig_defaults();
        
        // Generate build script
        generate_build_script();
        
        std::cout << "\nESP32 project setup complete!\n";
        std::cout << "\nNext steps:\n";
        std::cout << "1. Source ESP-IDF environment:\n";
        std::cout << "   source " << config.idf_path << "/export.sh\n";
        std::cout << "2. Build project:\n";
        std::cout << "   cd " << project_path << " && idf.py build\n";
        std::cout << "3. Flash to device:\n";
        std::cout << "   idf.py -p /dev/ttyUSB0 flash monitor\n";
        
        return true;
    }
    
private:
    void generate_root_cmake() {
        std::ostringstream cmake;
        cmake << "# Forma Generated ESP32 Project\n";
        cmake << "cmake_minimum_required(VERSION 3.16)\n\n";
        cmake << "# Set target before including project.cmake\n";
        cmake << "set(IDF_TARGET \"" << config.target << "\")\n\n";
        cmake << "include($ENV{IDF_PATH}/tools/cmake/project.cmake)\n";
        cmake << "project(forma-esp32-app)\n";
        
        std::ofstream file(project_path + "/CMakeLists.txt");
        file << cmake.str();
        file.close();
    }
    
    void generate_main_cmake() {
        std::ostringstream cmake;
        cmake << "idf_component_register(\n";
        cmake << "    SRCS \"main.c\"\n";
        cmake << "    INCLUDE_DIRS \".\"\n";
        cmake << ")\n";
        
        std::ofstream file(project_path + "/main/CMakeLists.txt");
        file << cmake.str();
        file.close();
    }
    
    void generate_main_c() {
        std::ostringstream main;
        main << "#include <stdio.h>\n";
        main << "#include \"freertos/FreeRTOS.h\"\n";
        main << "#include \"freertos/task.h\"\n";
        main << "#include \"esp_system.h\"\n";
        main << "#include \"esp_log.h\"\n\n";
        main << "static const char *TAG = \"FormaApp\";\n\n";
        main << "void app_main(void) {\n";
        main << "    ESP_LOGI(TAG, \"Starting Forma ESP32 Application\");\n";
        main << "    ESP_LOGI(TAG, \"Free heap: %lu bytes\", esp_get_free_heap_size());\n\n";
        main << "    // Your Forma-generated code will be integrated here\n";
        main << "    while (1) {\n";
        main << "        ESP_LOGI(TAG, \"Hello from Forma!\");\n";
        main << "        vTaskDelay(pdMS_TO_TICKS(1000));\n";
        main << "    }\n";
        main << "}\n";
        
        std::ofstream file(project_path + "/main/main.c");
        file << main.str();
        file.close();
    }
    
    void generate_sdkconfig_defaults() {
        std::ostringstream cfg;
        cfg << "# Forma ESP32 Configuration\n\n";
        cfg << "# Target configuration\n";
        cfg << "CONFIG_IDF_TARGET=\"" << config.target << "\"\n\n";
        cfg << "# Logging\n";
        cfg << "CONFIG_LOG_DEFAULT_LEVEL_INFO=y\n";
        cfg << "CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y\n\n";
        cfg << "# FreeRTOS\n";
        cfg << "CONFIG_FREERTOS_HZ=1000\n\n";
        cfg << "# Main task\n";
        cfg << "CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192\n";
        
        std::ofstream file(project_path + "/sdkconfig.defaults");
        file << cfg.str();
        file.close();
    }
    
    void generate_build_script() {
        std::ostringstream script;
        script << "#!/bin/bash\n";
        script << "# Forma ESP32 Build Script\n\n";
        script << "# Source ESP-IDF environment\n";
        script << "if [ -z \"$IDF_PATH\" ]; then\n";
        script << "    echo \"Setting up ESP-IDF environment...\"\n";
        script << "    source " << config.idf_path << "/export.sh\n";
        script << "fi\n\n";
        script << "# Build project\n";
        script << "idf.py build\n";
        
        std::string script_path = project_path + "/build.sh";
        std::ofstream file(script_path);
        file << script.str();
        file.close();
        
        // Make executable
        chmod(script_path.c_str(), 0755);
    }
};

// Helper to parse ESP32 configuration from TOML
inline bool parse_esp32_config(const std::string& toml_content, ESP32Config& config) {
    // Simple TOML parser for [esp32] section
    std::istringstream stream(toml_content);
    std::string line;
    bool in_esp32_section = false;
    
    while (std::getline(stream, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line[start] == '#') continue;
        
        // Check for [esp32] section
        if (line.find("[esp32]") != std::string::npos) {
            in_esp32_section = true;
            continue;
        }
        
        // Exit section on new section
        if (line[0] == '[') {
            in_esp32_section = false;
            continue;
        }
        
        if (!in_esp32_section) continue;
        
        // Parse key = value
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim
        key.erase(key.find_last_not_of(" \t") + 1);
        size_t vstart = value.find_first_not_of(" \t\\\"");
        if (vstart != std::string::npos) {
            value = value.substr(vstart);
        }
        if (!value.empty() && value.back() == '"') value.pop_back();
        
        if (key == "idf_version") config.idf_version = value;
        else if (key == "target") config.target = value;
        else if (key == "idf_path") config.idf_path = value;
        else if (key == "auto_install") config.auto_install = (value == "true");
        else if (key == "download_toolchain") config.download_toolchain = (value == "true");
    }
    
    return true;
}

} // namespace forma::esp32
