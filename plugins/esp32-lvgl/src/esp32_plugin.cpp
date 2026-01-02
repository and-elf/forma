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
    if (config_file) {
        forma::fs::RealFileSystem realfs;
        if (realfs.exists(config_file)) {
            std::cout << "Loading configuration from: " << config_file << "\n";
            auto toml_content = realfs.read_file(config_file);
            forma::esp32::parse_esp32_config(toml_content, config);
        }
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

// Plugin interface for forma build command
int forma_build(const char* project_dir, const char* config_path, bool verbose, bool flash, bool monitor) {
    if (!project_dir) {
        if (verbose) {
            std::cerr << "Error: Invalid project directory\n";
        }
        return 1;
    }
    
    if (verbose) {
        std::cout << "ESP32-LVGL build plugin\n";
        std::cout << "Project: " << project_dir << "\n";
        if (config_path) {
            std::cout << "Config: " << config_path << "\n";
        }
    }
    
    forma::esp32::ESP32BuildSystem builder(project_dir);
    forma::esp32::ESP32Config config;
    
    // Load configuration from TOML if provided
    if (config_path) {
        forma::fs::RealFileSystem realfs;
        if (realfs.exists(config_path)) {
            auto toml_content = realfs.read_file(config_path);
            forma::esp32::parse_esp32_config(toml_content, config);
            
            if (verbose) {
                std::cout << "Loaded ESP32 configuration:\n";
                std::cout << "  IDF Version: " << config.idf_version << "\n";
                std::cout << "  Target: " << config.target << "\n";
                if (!config.idf_path.empty()) {
                    std::cout << "  IDF Path: " << config.idf_path << "\n";
                }
            }
        }
    }
        
        if (verbose) {
            std::cout << "Loaded ESP32 configuration:\n";
            std::cout << "  IDF Version: " << config.idf_version << "\n";
            std::cout << "  Target: " << config.target << "\n";
            if (!config.idf_path.empty()) {
                std::cout << "  IDF Path: " << config.idf_path << "\n";
            }
        }
    }
    
    builder.set_config(config);
    
    // Setup the project (ESP-IDF, toolchain, etc.)
    if (verbose) {
        std::cout << "Setting up ESP32 project...\n";
    }
    
    if (!builder.setup_project()) {
        if (verbose) {
            std::cerr << "Error: Failed to setup ESP32 project\n";
        }
        return 1;
    }
    
    // Build the project
    if (verbose) {
        std::cout << "Building ESP32 project...\n";
    }
    
    if (!builder.build()) {
        if (verbose) {
            std::cerr << "Error: Build failed\n";
        }
        return 1;
    }
    
    if (verbose) {
        std::cout << "Build complete!\n";
    }
    
    // Flash if requested
    if (flash) {
        if (verbose) {
            std::cout << "Flashing to device...\n";
        }
        
        if (!builder.flash()) {
            if (verbose) {
                std::cerr << "Error: Flash failed\n";
            }
            return 1;
        }
        
        if (verbose) {
            std::cout << "Flash complete!\n";
        }
    }
    
    // Start monitor if requested
    if (monitor) {
        if (verbose) {
            std::cout << "Starting serial monitor...\n";
        }
        
        if (!builder.monitor()) {
            if (verbose) {
                std::cerr << "Error: Monitor failed\n";
            }
            return 1;
        }
    }
    
    return 0;
}

// Host-aware build variant
int forma_build_host(void* host_ptr, const char* project_dir, const char* config_path, bool verbose, bool flash, bool monitor) {
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!project_dir) {
        if (verbose) {
            std::cerr << "Error: Invalid project directory\n";
        }
        return 1;
    }

    if (verbose) {
        std::cout << "ESP32-LVGL build plugin (host-aware)\n";
        std::cout << "Project: " << project_dir << "\n";
        if (config_path) {
            std::cout << "Config: " << config_path << "\n";
        }
    }

    forma::esp32::ESP32BuildSystem builder(project_dir);
    forma::esp32::ESP32Config config;

    // Load configuration from TOML via host filesystem or stream_io
    if (config_path && host) {
        if (host->filesystem && host->filesystem->exists(config_path)) {
            auto toml_content = host->filesystem->read_file(config_path);
            forma::esp32::parse_esp32_config(toml_content, config);
        } else if (host->stream_io.open_read(config_path)) {
            auto toml_content = *host->stream_io.open_read(config_path);
            forma::esp32::parse_esp32_config(toml_content, config);
        }
        if (verbose) {
            std::cout << "Loaded ESP32 configuration:\n";
            std::cout << "  IDF Version: " << config.idf_version << "\n";
            std::cout << "  Target: " << config.target << "\n";
            if (!config.idf_path.empty()) {
                std::cout << "  IDF Path: " << config.idf_path << "\n";
            }
        }
    }

    builder.set_config(config);

    if (verbose) {
        std::cout << "Setting up ESP32 project...\n";
    }

    if (!builder.setup_project()) {
        if (verbose) {
            std::cerr << "Error: Failed to setup ESP32 project\n";
        }
        return 1;
    }

    if (verbose) {
        std::cout << "Building ESP32 project...\n";
    }

    if (!builder.build()) {
        if (verbose) {
            std::cerr << "Error: Build failed\n";
        }
        return 1;
    }

    if (verbose) {
        std::cout << "Build complete!\n";
    }

    if (flash) {
        if (verbose) std::cout << "Flashing to device...\n";
        if (!builder.flash()) {
            if (verbose) std::cerr << "Error: Flash failed\n";
            return 1;
        }
        if (verbose) std::cout << "Flash complete!\n";
    }

    if (monitor) {
        if (verbose) std::cout << "Starting serial monitor...\n";
        if (!builder.monitor()) {
            if (verbose) std::cerr << "Error: Monitor failed\n";
            return 1;
        }
    }

    return 0;
}

} // extern "C"
