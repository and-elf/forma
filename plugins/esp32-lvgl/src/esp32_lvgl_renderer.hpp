#pragma once

#include <string>
#include <sstream>
#include <fstream>

namespace forma::esp32 {

class ESP32LVGLRenderer {
private:
    std::ostringstream header;
    std::ostringstream source;
    std::ostringstream init_code;
    std::string output_path;
    
public:
    ESP32LVGLRenderer(const std::string& output_path) 
        : output_path(output_path) {
        generate_header();
    }
    
    void generate_header() {
        header << "#ifndef FORMA_UI_H\n";
        header << "#define FORMA_UI_H\n\n";
        header << "#include <stdio.h>\n";
        header << "#include \"freertos/FreeRTOS.h\"\n";
        header << "#include \"freertos/task.h\"\n";
        header << "#include \"esp_system.h\"\n";
        header << "#include \"esp_log.h\"\n";
        header << "#include \"lvgl.h\"\n";
        header << "#include \"esp_lvgl_port.h\"\n\n";
        header << "#ifdef __cplusplus\n";
        header << "extern \"C\" {\n";
        header << "#endif\n\n";
        header << "void forma_ui_init(void);\n";
        header << "void forma_ui_task(void *pvParameters);\n\n";
        header << "#ifdef __cplusplus\n";
        header << "}\n";
        header << "#endif\n\n";
        header << "#endif // FORMA_UI_H\n";
    }
    
    void generate_esp32_init() {
        source << "#include \"forma_ui.h\"\n\n";
        source << "static const char *TAG = \"FormaUI\";\n\n";
        
        // LVGL initialization
        source << "void forma_ui_init(void) {\n";
        source << "    ESP_LOGI(TAG, \"Initializing LVGL\");\n\n";
        source << "    // Initialize LVGL\n";
        source << "    lv_init();\n\n";
        source << "    // Initialize display\n";
        source << "    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();\n";
        source << "    lvgl_port_init(&lvgl_cfg);\n\n";
        source << "    // Create UI elements\n";
        source << init_code.str();
        source << "\n    ESP_LOGI(TAG, \"UI initialized successfully\");\n";
        source << "}\n\n";
        
        // LVGL task
        source << "void forma_ui_task(void *pvParameters) {\n";
        source << "    ESP_LOGI(TAG, \"Starting LVGL task\");\n";
        source << "    \n";
        source << "    forma_ui_init();\n\n";
        source << "    while (1) {\n";
        source << "        // Let LVGL handle its tasks\n";
        source << "        lv_task_handler();\n";
        source << "        vTaskDelay(pdMS_TO_TICKS(10));\n";
        source << "    }\n";
        source << "}\n";
    }
    
    void add_button(const std::string& name, const std::string& text, 
                    int x, int y, int width, int height) {
        std::string var_name = "btn_" + name;
        std::string label_name = "label_" + name;
        
        init_code << "    // Create button: " << name << "\n";
        init_code << "    lv_obj_t *" << var_name << " = lv_btn_create(lv_scr_act());\n";
        init_code << "    lv_obj_set_pos(" << var_name << ", " << x << ", " << y << ");\n";
        init_code << "    lv_obj_set_size(" << var_name << ", " << width << ", " << height << ");\n\n";
        init_code << "    lv_obj_t *" << label_name << " = lv_label_create(" << var_name << ");\n";
        init_code << "    lv_label_set_text(" << label_name << ", \"" << text << "\");\n";
        init_code << "    lv_obj_center(" << label_name << ");\n\n";
    }
    
    void add_label(const std::string& name, const std::string& text, int x, int y) {
        std::string var_name = "label_" + name;
        
        init_code << "    // Create label: " << name << "\n";
        init_code << "    lv_obj_t *" << var_name << " = lv_label_create(lv_scr_act());\n";
        init_code << "    lv_label_set_text(" << var_name << ", \"" << text << "\");\n";
        init_code << "    lv_obj_set_pos(" << var_name << ", " << x << ", " << y << ");\n\n";
    }
    
    void add_slider(const std::string& name, int x, int y, int width, 
                    int min_val, int max_val, int default_val) {
        std::string var_name = "slider_" + name;
        
        init_code << "    // Create slider: " << name << "\n";
        init_code << "    lv_obj_t *" << var_name << " = lv_slider_create(lv_scr_act());\n";
        init_code << "    lv_obj_set_pos(" << var_name << ", " << x << ", " << y << ");\n";
        init_code << "    lv_obj_set_width(" << var_name << ", " << width << ");\n";
        init_code << "    lv_slider_set_range(" << var_name << ", " << min_val << ", " << max_val << ");\n";
        init_code << "    lv_slider_set_value(" << var_name << ", " << default_val << ", LV_ANIM_OFF);\n\n";
    }
    
    void generate_cmake() {
        std::ostringstream cmake;
        cmake << "# Forma Generated ESP32 Project\n";
        cmake << "cmake_minimum_required(VERSION 3.16)\n\n";
        cmake << "include($ENV{IDF_PATH}/tools/cmake/project.cmake)\n";
        cmake << "project(forma-esp32-app)\n";
    }
    
    void generate_main_component() {
        std::ostringstream main_c;
        main_c << "#include <stdio.h>\n";
        main_c << "#include \"freertos/FreeRTOS.h\"\n";
        main_c << "#include \"freertos/task.h\"\n";
        main_c << "#include \"esp_log.h\"\n";
        main_c << "#include \"forma_ui.h\"\n\n";
        main_c << "static const char *TAG = \"main\";\n\n";
        main_c << "void app_main(void) {\n";
        main_c << "    ESP_LOGI(TAG, \"Starting Forma ESP32 Application\");\n\n";
        main_c << "    // Create LVGL UI task\n";
        main_c << "    xTaskCreate(forma_ui_task, \"lvgl_task\", 8192, NULL, 5, NULL);\n";
        main_c << "}\n";
    }
    
    bool write_files() {
        generate_esp32_init();
        
        // Write header file
        std::ofstream header_file(output_path + "/forma_ui.h");
        if (!header_file) return false;
        header_file << header.str();
        header_file.close();
        
        // Write source file
        std::ofstream source_file(output_path + "/forma_ui.c");
        if (!source_file) return false;
        source_file << source.str();
        source_file.close();
        
        return true;
    }
};

} // namespace forma::esp32
