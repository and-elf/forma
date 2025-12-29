#include "esp32_lvgl_renderer.hpp"
#include <iostream>
#include <filesystem>

extern "C" {

const char* get_plugin_name() {
    return "esp32-lvgl";
}

const char* get_plugin_version() {
    return "0.1.0";
}

const char* get_plugin_description() {
    return "Generates ESP-IDF compatible C code with LVGL for ESP32 microcontrollers";
}

const char* get_plugin_type() {
    return "renderer";
}

// Main rendering function
bool render_esp32_lvgl(const char* output_dir) {
    if (!output_dir) {
        std::cerr << "Error: Invalid output directory\n";
        return false;
    }
    
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_dir);
    
    std::cout << "Generating ESP32 LVGL code in: " << output_dir << "\n";
    
    forma::esp32::ESP32LVGLRenderer renderer(output_dir);
    
    // Example UI generation (this would normally parse Forma IR)
    renderer.add_label("title", "ESP32 Forma App", 10, 10);
    renderer.add_button("btn1", "Click Me", 50, 50, 120, 50);
    renderer.add_slider("brightness", 50, 120, 200, 0, 100, 50);
    
    if (!renderer.write_files()) {
        std::cerr << "Error: Failed to write output files\n";
        return false;
    }
    
    std::cout << "Generated files:\n";
    std::cout << "  - forma_ui.h\n";
    std::cout << "  - forma_ui.c\n";
    std::cout << "\nNext steps:\n";
    std::cout << "1. Copy these files to your ESP-IDF project's main/ directory\n";
    std::cout << "2. Add LVGL component to your project\n";
    std::cout << "3. Configure display driver in menuconfig\n";
    std::cout << "4. Build with: idf.py build\n";
    std::cout << "5. Flash with: idf.py flash monitor\n";
    
    return true;
}

} // extern "C"
