# ESP32 LVGL Renderer Plugin for Forma
# Generates ESP-IDF compatible C code with LVGL for ESP32 microcontrollers

cmake_minimum_required(VERSION 3.20)
project(FormaESP32LVGLPlugin VERSION 0.1.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Plugin library
add_library(forma_esp32_lvgl SHARED
    src/esp32_build_system.hpp
    src/esp32_plugin.cpp
)

set_target_properties(forma_esp32_lvgl PROPERTIES
    PREFIX ""
    OUTPUT_NAME "forma-esp32-lvgl"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(forma_esp32_lvgl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/plugins/esp32-lvgl>
)

# Installation
install(TARGETS forma_esp32_lvgl
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
)

install(FILES src/esp32_lvgl_renderer.hpp
    DESTINATION include/forma/plugins/esp32-lvgl
)
