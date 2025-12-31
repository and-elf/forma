#pragma once

#include <parser/ir.hpp>
#include <array>
#include <string_view>

namespace forma::lvgl {

// ============================================================================
// Asset Bundling Support
// ============================================================================

// Asset types supported by LVGL
enum class AssetType {
    Image,    // PNG, JPG, BMP (converted to C array)
    Font,     // TTF fonts (converted to LVGL font)
    Binary    // Raw binary data
};

// ============================================================================
// Platform Configuration
// ============================================================================

enum class Platform {
    FreeRTOS,
    ZephyrRTOS,
    Windows,
    Linux
};

// ============================================================================
// LVGL C Code Generator
// ============================================================================

template <size_t MaxOutput = 16384>
class LVGLRenderer {
private:
    std::array<char, MaxOutput> output_buffer{};
    size_t output_pos = 0;
    size_t indent_level = 0;
    size_t callback_count = 0;  // Track number of callbacks generated
    Platform target_platform = Platform::Linux;  // Default platform
    
    // Mapping from Forma types to LVGL widget types
    constexpr const char* map_type_to_lvgl(std::string_view type_name) const {
        if (type_name == "Button") return "lv_btn";
        if (type_name == "Label") return "lv_label";
        if (type_name == "Panel" || type_name == "Container") return "lv_obj";
        if (type_name == "Slider") return "lv_slider";
        if (type_name == "Switch") return "lv_switch";
        if (type_name == "Checkbox") return "lv_checkbox";
        if (type_name == "Dropdown") return "lv_dropdown";
        if (type_name == "TextArea") return "lv_textarea";
        if (type_name == "Image") return "lv_img";
        if (type_name == "Arc") return "lv_arc";
        if (type_name == "Bar") return "lv_bar";
        if (type_name == "Spinner") return "lv_spinner";
        if (type_name == "List") return "lv_list";
        if (type_name == "Chart") return "lv_chart";
        if (type_name == "Table") return "lv_table";
        if (type_name == "Calendar") return "lv_calendar";
        if (type_name == "Keyboard") return "lv_keyboard";
        if (type_name == "Roller") return "lv_roller";
        if (type_name == "Textarea") return "lv_textarea";
        
        // Default to generic object
        return "lv_obj";
    }
    
    // Map Forma property names to LVGL API calls
    constexpr const char* map_property_to_lvgl_setter(std::string_view prop_name) const {
        if (prop_name == "text") return "lv_label_set_text";
        if (prop_name == "width") return "lv_obj_set_width";
        if (prop_name == "height") return "lv_obj_set_height";
        if (prop_name == "x") return "lv_obj_set_x";
        if (prop_name == "y") return "lv_obj_set_y";
        if (prop_name == "visible") return "lv_obj_set_hidden";
        if (prop_name == "enabled") return "lv_obj_set_enabled";
        if (prop_name == "value") return "lv_slider_set_value";
        if (prop_name == "min") return "lv_slider_set_range";
        if (prop_name == "max") return "lv_slider_set_range";
        if (prop_name == "checked") return "lv_checkbox_set_checked";
        
        return nullptr;  // Unknown property
    }
    
    constexpr void append(const char* str) {
        while (*str && output_pos < MaxOutput - 1) {
            output_buffer[output_pos++] = *str++;
        }
    }
    
    constexpr void append(std::string_view str) {
        for (char c : str) {
            if (output_pos >= MaxOutput - 1) break;
            output_buffer[output_pos++] = c;
        }
    }
    
    constexpr void append_line(const char* str = "") {
        // Add indentation
        for (size_t i = 0; i < indent_level; ++i) {
            append("    ");
        }
        append(str);
        append("\n");
    }
    
    // Convert forma://path/to/file.ext to asset_path_to_file_ext
    constexpr void generate_asset_symbol_name(std::string_view uri) {
        append("asset_");
        
        // Skip "forma://" prefix
        size_t start = uri.find("://");
        if (start != std::string_view::npos) {
            start += 3;
        } else {
            start = 0;
        }
        
        // Convert path to valid C identifier
        for (size_t i = start; i < uri.size() && output_pos < MaxOutput - 1; ++i) {
            char c = uri[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                output_buffer[output_pos++] = (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
            } else {
                output_buffer[output_pos++] = '_';
            }
        }
    }
    
    constexpr void generate_platform_includes() {
        append_line("/* Platform-specific includes */");
        
        if (target_platform == Platform::FreeRTOS) {
            append_line("#include \"FreeRTOS.h\"");
            append_line("#include \"task.h\"");
        } else if (target_platform == Platform::ZephyrRTOS) {
            append_line("#include <zephyr/kernel.h>");
        } else if (target_platform == Platform::Windows) {
            append_line("#include <windows.h>");
        } else if (target_platform == Platform::Linux) {
            append_line("#include <unistd.h>");
        }
        append_line();
    }
    
    constexpr void append_int(int value) {
        if (value == 0) {
            append("0");
            return;
        }
        
        char temp[32];
        int idx = 0;
        bool negative = value < 0;
        if (negative) value = -value;
        
        while (value > 0) {
            temp[idx++] = '0' + (value % 10);
            value /= 10;
        }
        
        if (negative) {
            append("-");
        }
        
        while (idx > 0) {
            if (output_pos < MaxOutput - 1) {
                output_buffer[output_pos++] = temp[--idx];
            }
        }
    }
    
    constexpr void generate_header() {
        append_line("/**");
        append_line(" * Generated by Forma LVGL Renderer");
        append_line(" * This file contains LVGL UI code generated from .fml definitions");
        append_line(" */");
        append_line();
        append_line("#include \"lvgl.h\"");
        append_line("#include <stdint.h>");
        append_line("#include <stdbool.h>");
        append_line();
    }
    
    // Generate asset declarations (extern references to bundled data)
    template<typename DocType>
    constexpr void generate_asset_declarations(const DocType& document) {
        if (document.asset_count == 0) return;
        
        append_line("/* Bundled Assets */");
        
        for (size_t i = 0; i < document.asset_count; ++i) {
            const auto& asset = document.assets[i];
            
            // Declare as extern const array
            append("extern const unsigned char ");
            generate_asset_symbol_name(asset.uri);
            append("[];");
            append_line();
            
            append("extern const unsigned int ");
            generate_asset_symbol_name(asset.uri);
            append("_size;");
            append_line();
        }
        
        append_line();
    }
    
    constexpr void generate_variable_name(std::string_view type_name, size_t instance_idx) {
        // Convert TypeName to type_name_N format
        append("lv_obj_t *");
        generate_variable_name_only(type_name, instance_idx);
    }
    
    constexpr void generate_variable_name_only(std::string_view type_name, size_t instance_idx) {
        // Convert TypeName to type_name_N format (without lv_obj_t *)
        bool first = true;
        for (char c : type_name) {
            if (output_pos >= MaxOutput - 1) break;
            
            // Skip invalid characters
            if (c == '{' || c == '}' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                continue;
            }
            
            if (c >= 'A' && c <= 'Z') {
                if (!first) {
                    output_buffer[output_pos++] = '_';
                }
                output_buffer[output_pos++] = c + ('a' - 'A');  // to lowercase
            } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                output_buffer[output_pos++] = c;
            }
            first = false;
        }
        
        append("_");
        append_int(instance_idx);
    }
    
    constexpr void generate_instance_creation(const InstanceNode* instances,
                                              const InstanceDecl& inst, 
                                              size_t inst_idx,
                                              size_t parent_idx = 0) {
        const char* lvgl_type = map_type_to_lvgl(inst.type_name);
        
        // Generate assignment (widget variables are now static globals)
        for (size_t i = 0; i < indent_level; ++i) {
            append("    ");
        }
        
        generate_variable_name_only(inst.type_name, inst_idx);
        append(" = ");
        append(lvgl_type);
        append("_create(");
        
        if (parent_idx == 0) {
            append("lv_scr_act()");  // Root screen
        } else {
            // Reference parent object by its generated variable name
            const auto& parent = instances->get(parent_idx);
            generate_variable_name_only(parent.type_name, parent_idx);
        }
        
        append(");\n");
    }
    
    constexpr void generate_property_setter(const PropertyAssignment& prop,
                                           size_t inst_idx,
                                           std::string_view type_name) {
        const char* setter = map_property_to_lvgl_setter(prop.name);
        
        // Special handling for image source property
        if (prop.name == "src" && type_name == "Image") {
            for (size_t i = 0; i < indent_level; ++i) append("    ");
            append("lv_img_set_src(");
            generate_variable_name_only(type_name, inst_idx);
            append(", ");
            
            // Check if value is a forma:// URI
            if (prop.value.kind == Value::Kind::String || prop.value.kind == Value::Kind::URI) {
                if (prop.value.text.size() >= 8 && 
                    prop.value.text.substr(0, 8) == "forma://") {
                    // Reference bundled asset
                    generate_asset_symbol_name(prop.value.text);
                } else {
                    // Regular string path (file system)
                    append("\"");
                    append(prop.value.text);
                    append("\"");
                }
            }
            
            append(");");
            append_line();
            return;
        }
        
        if (!setter) {
            // Try generic position/size setters
            if (prop.name == "x" || prop.name == "y" || prop.name == "width" || prop.name == "height") {
                for (size_t i = 0; i < indent_level; ++i) append("    ");
                append("lv_obj_set_");
                append(prop.name);
                append("(");
                generate_variable_name_only(type_name, inst_idx);
                append(", ");
                append(prop.value.text);
                append(");\n");
            }
            return;
        }
        
        // Generate the setter call
        for (size_t i = 0; i < indent_level; ++i) {
            append("    ");
        }
        
        append(setter);
        append("(");
        generate_variable_name_only(type_name, inst_idx);
        append(", ");
        
        // Generate the value
        if (prop.value.kind == Value::Kind::String) {
            append("\"");
            append(prop.value.text);
            append("\"");
        } else if (prop.value.kind == Value::Kind::Bool) {
            append(prop.value.text);
        } else {
            append(prop.value.text);
        }
        
        append(");\n");
    }
    
    // Map event names to LVGL event codes
    constexpr const char* map_event_to_lvgl(std::string_view event_name) const {
        if (event_name == "onClick" || event_name == "clicked") return "LV_EVENT_CLICKED";
        if (event_name == "onPressed" || event_name == "pressed") return "LV_EVENT_PRESSED";
        if (event_name == "onReleased" || event_name == "released") return "LV_EVENT_RELEASED";
        if (event_name == "onValueChanged" || event_name == "value_changed") return "LV_EVENT_VALUE_CHANGED";
        if (event_name == "onFocused" || event_name == "focused") return "LV_EVENT_FOCUSED";
        if (event_name == "onDefocused" || event_name == "defocused") return "LV_EVENT_DEFOCUSED";
        return "LV_EVENT_CLICKED";  // Default
    }
    
    // Generate callback function for when blocks
    constexpr void generate_callback_function(const WhenStmt& when_stmt,
                                              size_t inst_idx,
                                              std::string_view type_name,
                                              size_t callback_idx) {
        // Generate callback function name
        append("static void ");
        generate_variable_name_only(type_name, inst_idx);
        append("_callback_");
        append_int(callback_idx);
        append("(lv_event_t* e) {\n");
        indent_level++;
        
        append_line("lv_event_code_t code = lv_event_get_code(e);");
        append_line("lv_obj_t* obj = lv_event_get_target(e);");
        append_line();
        
        // Check event condition if specified
        append_line("/* Condition-based updates */");
        
        // For now, generate all assignments unconditionally
        // TODO: Parse condition and generate proper if/else
        for (size_t i = 0; i < when_stmt.assignment_count; ++i) {
            const auto& assign = when_stmt.assignments[i];
            
            // Generate setter for each assignment
            for (size_t j = 0; j < indent_level; ++j) append("    ");
            append("/* TODO: Set ");
            append(assign.name);
            append(" to ");
            append(assign.value.text);
            append(" */\n");
        }
        
        indent_level--;
        append_line("}");
        append_line();
    }
    
    // Generate event handler attachment
    constexpr void generate_event_handler(std::string_view event_name,
                                          size_t inst_idx,
                                          std::string_view type_name,
                                          size_t callback_idx) {
        const char* lvgl_event = map_event_to_lvgl(event_name);
        
        for (size_t i = 0; i < indent_level; ++i) {
            append("    ");
        }
        
        append("lv_obj_add_event_cb(");
        generate_variable_name_only(type_name, inst_idx);
        append(", ");
        generate_variable_name_only(type_name, inst_idx);
        append("_callback_");
        append_int(callback_idx);
        append(", ");
        append(lvgl_event);
        append(", NULL);\n");
    }
    
    // Map easing names to LVGL animation paths
    constexpr const char* map_easing_to_lvgl(std::string_view easing) const {
        if (easing.empty() || easing == "linear") return "lv_anim_path_linear";
        if (easing == "ease_in") return "lv_anim_path_ease_in";
        if (easing == "ease_out") return "lv_anim_path_ease_out";
        if (easing == "ease_in_out") return "lv_anim_path_ease_in_out";
        if (easing == "overshoot") return "lv_anim_path_overshoot";
        if (easing == "bounce") return "lv_anim_path_bounce";
        return "lv_anim_path_linear";  // Default
    }
    
    // Map property names to LVGL animation setter callbacks
    constexpr const char* map_property_to_anim_setter(std::string_view prop_name) const {
        if (prop_name == "x") return "lv_obj_set_x";
        if (prop_name == "y") return "lv_obj_set_y";
        if (prop_name == "width") return "lv_obj_set_width";
        if (prop_name == "height") return "lv_obj_set_height";
        if (prop_name == "opacity") return "lv_obj_set_style_opa";
        return nullptr;
    }
    
    // Generate animation initialization
    constexpr void generate_animation(const AnimationDecl& anim_decl,
                                      size_t inst_idx,
                                      std::string_view type_name,
                                      size_t anim_idx) {
        const char* setter = map_property_to_anim_setter(anim_decl.target_property);
        if (!setter) return;  // Unknown property
        
        // Generate animation variable
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_t anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(";\n");
        
        // Initialize animation
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_init(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(");\n");
        
        // Set animation target
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_set_var(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(", ");
        generate_variable_name_only(type_name, inst_idx);
        append(");\n");
        
        // Set start and end values
        if (anim_decl.start_value.kind != Value::Kind::Integer || anim_decl.end_value.kind != Value::Kind::Integer) {
            return;  // Only support integer values for now
        }
        
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_set_values(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(", ");
        append(anim_decl.start_value.text);
        append(", ");
        append(anim_decl.end_value.text);
        append(");\n");
        
        // Set duration
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_set_time(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(", ");
        append_int(anim_decl.duration_ms);
        append(");\n");
        
        // Set delay if specified
        if (anim_decl.delay_ms > 0) {
            for (size_t i = 0; i < indent_level; ++i) append("    ");
            append("lv_anim_set_delay(&anim_");
            generate_variable_name_only(type_name, inst_idx);
            append("_");
            append_int(anim_idx);
            append(", ");
            append_int(anim_decl.delay_ms);
            append(");\n");
        }
        
        // Set repeat if specified
        if (anim_decl.repeat) {
            for (size_t i = 0; i < indent_level; ++i) append("    ");
            append("lv_anim_set_repeat_count(&anim_");
            generate_variable_name_only(type_name, inst_idx);
            append("_");
            append_int(anim_idx);
            append(", LV_ANIM_REPEAT_INFINITE);\n");
        }
        
        // Set easing path
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_set_path_cb(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(", ");
        append(map_easing_to_lvgl(anim_decl.easing));
        append(");\n");
        
        // Set exec callback (the setter function)
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_set_exec_cb(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(", (lv_anim_exec_xcb_t)");
        append(setter);
        append(");\n");
        
        // Start animation
        for (size_t i = 0; i < indent_level; ++i) append("    ");
        append("lv_anim_start(&anim_");
        generate_variable_name_only(type_name, inst_idx);
        append("_");
        append_int(anim_idx);
        append(");\n");
    }
    
    constexpr void generate_instance_recursive(const InstanceNode& instances,
                                               size_t inst_idx,
                                               size_t parent_idx = 0) {
        const auto& inst = instances.get(inst_idx);
        
        // Create the instance
        generate_instance_creation(&instances, inst, inst_idx, parent_idx);
        
        // Set properties
        for (size_t i = 0; i < inst.prop_count; ++i) {
            generate_property_setter(inst.properties[i], inst_idx, inst.type_name);
        }
        
        // Attach event handlers for when blocks
        for (size_t i = 0; i < inst.when_count; ++i) {
            // Extract event name from condition (simplified - assumes "eventName")
            generate_event_handler(inst.when_stmts[i].condition, inst_idx, inst.type_name, callback_count++);
        }
        
        // Generate animations
        for (size_t i = 0; i < inst.animation_count; ++i) {
            generate_animation(inst.animations[i], inst_idx, inst.type_name, i);
        }
        
        // Process children
        for (size_t i = 0; i < inst.child_count; ++i) {
            size_t child_idx = inst.child_indices[i];
            if (child_idx < instances.count) {
                generate_instance_recursive(instances, child_idx, inst_idx);
            }
        }
    }
    
    // Generate all callback functions (must be called before main function)
    constexpr void generate_all_callbacks(const InstanceNode& instances, size_t inst_idx = 0) {
        if (inst_idx >= instances.count) return;
        
        const auto& inst = instances.get(inst_idx);
        
        // Generate callbacks for this instance
        size_t local_callback_idx = 0;
        for (size_t i = 0; i < inst.when_count; ++i) {
            generate_callback_function(inst.when_stmts[i], inst_idx, inst.type_name, local_callback_idx++);
        }
        
        // Generate for children
        for (size_t i = 0; i < inst.child_count; ++i) {
            size_t child_idx = inst.child_indices[i];
            if (child_idx < instances.count) {
                generate_all_callbacks(instances, child_idx);
            }
        }
    }
    
    constexpr void generate_class_instances(const auto& document) {
        // Generate global instances for classes (TypeDecl with methods)
        bool has_classes = false;
        for (size_t i = 0; i < document.type_count; ++i) {
            if (document.types[i].method_count > 0) {
                has_classes = true;
                break;
            }
        }
        
        if (!has_classes) return;
        
        append_line("/* ============================================================================");
        append_line(" * Class Definitions (Public API)");
        append_line(" * ============================================================================ */");
        append_line();
        
        // Generate struct definitions for each class
        for (size_t i = 0; i < document.type_count; ++i) {
            const auto& type = document.types[i];
            if (type.method_count == 0) continue;  // Not a class
            
            // Generate typedef struct
            append("typedef struct {\n");
            indent_level++;
            
            // Generate properties
            for (size_t j = 0; j < type.prop_count; ++j) {
                const auto& prop = type.properties[j];
                for (size_t k = 0; k < indent_level; ++k) append("    ");
                append(map_type_to_c(prop.type));
                append(" ");
                append(prop.name);
                append(";\n");
            }
            
            indent_level--;
            append("} ");
            append(type.name);
            append(";\n\n");
        }
        
        // Generate global instances
        append_line("/* Class Instances (Global) */");
        for (size_t i = 0; i < document.type_count; ++i) {
            const auto& type = document.types[i];
            if (type.method_count == 0) continue;
            
            append(type.name);
            append(" ");
            // Convert type name to lowercase for instance name
            for (size_t j = 0; j < type.name.size() && output_pos < MaxOutput - 1; ++j) {
                char c = type.name[j];
                if (c >= 'A' && c <= 'Z') {
                    output_buffer[output_pos++] = c + ('a' - 'A');
                } else {
                    output_buffer[output_pos++] = c;
                }
            }
            append(" = {");
            
            // Initialize properties with default values
            bool first = true;
            for (size_t j = 0; j < type.prop_count; ++j) {
                if (!first) append(", ");
                first = false;
                
                append(".");
                append(type.properties[j].name);
                append(" = ");
                
                // Default initialization based on type
                if (type.properties[j].type.name == "int" || 
                    type.properties[j].type.name == "i32") {
                    append("0");
                } else if (type.properties[j].type.name == "bool") {
                    append("false");
                } else if (type.properties[j].type.name == "string") {
                    append("NULL");
                } else {
                    append("0");
                }
            }
            
            append("};\n");
        }
        append_line();
    }
    
    constexpr const char* map_type_to_c(const TypeRef& type) const {
        if (type.name == "int" || type.name == "i32") return "int32_t";
        if (type.name == "i64") return "int64_t";
        if (type.name == "i16") return "int16_t";
        if (type.name == "i8") return "int8_t";
        if (type.name == "u32") return "uint32_t";
        if (type.name == "u64") return "uint64_t";
        if (type.name == "u16") return "uint16_t";
        if (type.name == "u8") return "uint8_t";
        if (type.name == "f32" || type.name == "float") return "float";
        if (type.name == "f64" || type.name == "double") return "double";
        if (type.name == "bool") return "bool";
        if (type.name == "string") return "const char*";
        if (type.name == "void") return "void";
        
        // Unknown type, use as-is (might be a custom type)
        static char buffer[64];
        size_t len = type.name.size() < 63 ? type.name.size() : 63;
        for (size_t i = 0; i < len; ++i) {
            buffer[i] = type.name[i];
        }
        buffer[len] = '\0';
        return buffer;
    }
    
    constexpr void generate_class_methods([[maybe_unused]] const auto& document) {
        // Method declarations are generated in generate_class_instances()
        // Users must implement the methods in their own code
    }

public:
    constexpr LVGLRenderer() = default;
    
    
    constexpr void set_platform(Platform platform) {
        target_platform = platform;
    }
    
    // Generate C99 code for the entire document
    constexpr void generate(const auto& document) {
        output_pos = 0;
        indent_level = 0;
        callback_count = 0;
        
        generate_header();
        generate_platform_includes();
        generate_asset_declarations(document);
        
        // Generate type definitions as comments
        if (document.type_count > 0) {
            append_line("/* Type Definitions */");
            for (size_t i = 0; i < document.type_count; ++i) {
                const auto& type = document.types[i];
                append("/* type ");
                append(type.name);
                append(" { ");
                for (size_t j = 0; j < type.prop_count; ++j) {
                    if (j > 0) append(", ");
                    append(type.properties[j].name);
                    append(": ");
                    append(type.properties[j].type.name);
                }
                append(" } */\n");
            }
            append_line();
        }
        
        // Generate enum definitions
        if (document.enum_count > 0) {
            for (size_t i = 0; i < document.enum_count; ++i) {
                const auto& enum_decl = document.enums[i];
                append("typedef enum {\n");
                indent_level++;
                
                for (size_t j = 0; j < enum_decl.value_count; ++j) {
                    for (size_t k = 0; k < indent_level; ++k) append("    ");
                    append(enum_decl.name);
                    append("_");
                    append(enum_decl.values[j].name);
                    if (j < enum_decl.value_count - 1) {
                        append(",");
                    }
                    append("\n");
                }
                
                indent_level--;
                append("} ");
                append(enum_decl.name);
                append(";\n\n");
            }
        }
        
        // Generate callback functions before main UI function
        if (document.instances.count > 0) {
            append_line("/* Event Callbacks (Internal) */");
            for (size_t i = 0; i < document.instances.count; ++i) {
                if (i == 0) {  // Start from root
                    generate_all_callbacks(document.instances, i);
                    break;
                }
            }
        }
        
        // Generate class instances (public API)
        generate_class_instances(document);
        
        // Generate class method implementations
        generate_class_methods(document);
        
        // Generate UI widget variables (internal/private)
        if (document.instances.count > 0) {
            append_line("/* UI Widgets (Internal) */");
            for (size_t i = 0; i < document.instances.count; ++i) {
                append("static lv_obj_t *");
                generate_variable_name_only(document.instances.get(i).type_name, i);
                append(" = NULL;\n");
            }
            append_line();
        }
        
        // Generate forma_init function
        append_line("/**");
        append_line(" * Initialize the Forma UI system");
        append_line(" * Call this once during application startup");
        append_line(" */");
        append_line("void forma_init(void) {");
        indent_level++;
        
        // Generate instances
        if (document.instances.count > 0) {
            // Find root instances (those that are not children of any other instance)
            bool is_child[64] = {}; // Track which instances are children
            
            for (size_t i = 0; i < document.instances.count; ++i) {
                const auto& inst = document.instances.get(i);
                // Mark all children of this instance
                for (size_t j = 0; j < inst.child_count; ++j) {
                    size_t child_idx = inst.child_indices[j];
                    if (child_idx < 64) {
                        is_child[child_idx] = true;
                    }
                }
            }
            
            // Generate all root instances (and their children recursively)
            for (size_t i = 0; i < document.instances.count; ++i) {
                if (!is_child[i]) {
                    // This is a root instance
                    generate_instance_recursive(document.instances, i, 0);
                }
            }
        }
        
        indent_level--;
        append_line("}");
        append_line();
        
        // Generate forma_run function (platform-specific)
        append_line("/**");
        append_line(" * Run the Forma UI main loop");
        append_line(" * This function handles platform-specific event processing");
        append_line(" */");
        append_line("void forma_run(void) {");
        indent_level++;
        
        if (target_platform == Platform::FreeRTOS) {
            append_line("/* FreeRTOS: LVGL task loop */");
            append_line("while (1) {");
            indent_level++;
            append_line("lv_timer_handler();");
            append_line("vTaskDelay(pdMS_TO_TICKS(5));");
            indent_level--;
            append_line("}");
        } else if (target_platform == Platform::ZephyrRTOS) {
            append_line("/* Zephyr RTOS: LVGL task loop */");
            append_line("while (1) {");
            indent_level++;
            append_line("lv_timer_handler();");
            append_line("k_msleep(5);");
            indent_level--;
            append_line("}");
        } else if (target_platform == Platform::Windows) {
            append_line("/* Windows: LVGL event loop */");
            append_line("while (1) {");
            indent_level++;
            append_line("lv_timer_handler();");
            append_line("Sleep(5);");
            indent_level--;
            append_line("}");
        } else if (target_platform == Platform::Linux) {
            append_line("/* Linux: LVGL event loop */");
            append_line("while (1) {");
            indent_level++;
            append_line("lv_timer_handler();");
            append_line("usleep(5000);");
            indent_level--;
            append_line("}");
        }
        
        indent_level--;
        append_line("}");
        append_line();
        
        // Null-terminate
        if (output_pos < MaxOutput) {
            output_buffer[output_pos] = '\0';
        }
    }
    
    
    constexpr std::string_view get_output() const {
        return std::string_view(output_buffer.data(), output_pos);
    }
    
    constexpr const char* c_str() const {
        return output_buffer.data();
    }
};

} // namespace forma::lvgl
