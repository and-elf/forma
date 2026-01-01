#include "lvgl_native_renderer.hpp"
#include "sdl3_downloader.hpp"
#include "lvgl_downloader.hpp"
#include <iostream>

// Ensure SDL3 and LVGL are available when plugin is loaded
namespace {
    struct PluginInitializer {
        PluginInitializer() {
            std::cout << "Checking for SDL3 and LVGL dependencies...\n";
            
            auto sdl3_path = forma::sdl3::ensure_sdl3_available();
            if (sdl3_path.empty()) {
                std::cerr << "Warning: SDL3 not available\n";
            } else {
                std::cout << "SDL3 available at: " << sdl3_path << "\n";
            }
            
            auto lvgl_path = forma::lvgl::ensure_lvgl_available();
            if (lvgl_path.empty()) {
                std::cerr << "Warning: LVGL not available\n";
            } else {
                std::cout << "LVGL available at: " << lvgl_path << "\n";
            }
        }
    };
    
    static PluginInitializer g_init;
}
