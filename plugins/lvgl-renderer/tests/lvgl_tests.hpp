#pragma once

#include "lvgl_renderer.hpp"
#include "forma.hpp"
#include <cassert>
#include <iostream>

namespace forma::lvgl::tests {

// Test 1: Simple button with text
constexpr bool test_simple_button() {
    Document<4, 4, 4, 4, 4> doc;
    
    // Create a Button instance with text property
    InstanceDecl button;
    button.type_name = "Button";
    button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click Me"}, Value{}, false};
    button.prop_count = 1;
    
    doc.instances.add_instance(button);
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    // Verify output contains LVGL button creation
    bool has_lvgl_h = output.find("#include \"lvgl.h\"") != std::string_view::npos;
    bool has_btn_create = output.find("lv_btn_create") != std::string_view::npos;
    bool has_function = output.find("void forma_init(void)") != std::string_view::npos;
    
    return has_lvgl_h && has_btn_create && has_function;
}

// Test 2: Label with text property
constexpr bool test_label() {
    Document<4, 4, 4, 4, 4> doc;
    
    InstanceDecl label;
    label.type_name = "Label";
    label.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Hello LVGL"}, Value{}, false};
    label.prop_count = 1;
    
    doc.instances.add_instance(label);
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_label_create = output.find("lv_label_create") != std::string_view::npos;
    bool has_text_set = output.find("lv_label_set_text") != std::string_view::npos;
    
    return has_label_create && has_text_set;
}

// Test 3: Container with size properties
constexpr bool test_container_with_size() {
    Document<4, 4, 4, 4, 4> doc;
    
    InstanceDecl container;
    container.type_name = "Container";
    container.properties[0] = PropertyAssignment{"width", Value{Value::Kind::Integer, "200"}, Value{}, false};
    container.properties[1] = PropertyAssignment{"height", Value{Value::Kind::Integer, "100"}, Value{}, false};
    container.prop_count = 2;
    
    doc.instances.add_instance(container);
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_obj_create = output.find("lv_obj_create") != std::string_view::npos;
    bool has_width = output.find("lv_obj_set_width") != std::string_view::npos;
    bool has_height = output.find("lv_obj_set_height") != std::string_view::npos;
    
    return has_obj_create && has_width && has_height;
}

// Test 4: Enum generation
constexpr bool test_enum_generation() {
    Document<4, 4, 4, 4, 4> doc;
    
    EnumDecl color_enum;
    color_enum.name = "Color";
    color_enum.values[0] = EnumValue{"Red"};
    color_enum.values[1] = EnumValue{"Green"};
    color_enum.values[2] = EnumValue{"Blue"};
    color_enum.value_count = 3;
    
    doc.enums[0] = color_enum;
    doc.enum_count = 1;
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_typedef = output.find("typedef enum") != std::string_view::npos;
    bool has_red = output.find("Color_Red") != std::string_view::npos;
    bool has_green = output.find("Color_Green") != std::string_view::npos;
    bool has_blue = output.find("Color_Blue") != std::string_view::npos;
    
    return has_typedef && has_red && has_green && has_blue;
}

// Test 5: Multiple widgets
constexpr bool test_multiple_widgets() {
    Document<4, 4, 4, 4, 4> doc;
    
    InstanceDecl button;
    button.type_name = "Button";
    button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "OK"}, Value{}, false};
    button.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    button.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "20"}, Value{}, false};
    button.prop_count = 3;
    
    InstanceDecl label;
    label.type_name = "Label";
    label.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Status"}, Value{}, false};
    label.prop_count = 1;
    
    doc.instances.add_instance(button);
    
    LVGLRenderer<4096> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_button = output.find("lv_btn_create") != std::string_view::npos;
    bool has_set_x = output.find("lv_obj_set_x") != std::string_view::npos;
    bool has_set_y = output.find("lv_obj_set_y") != std::string_view::npos;
    
    return has_button && has_set_x && has_set_y;
}

// Test 6: Slider widget
constexpr bool test_slider() {
    Document<4, 4, 4, 4, 4> doc;
    
    InstanceDecl slider;
    slider.type_name = "Slider";
    slider.properties[0] = PropertyAssignment{"value", Value{Value::Kind::Integer, "50"}, Value{}, false};
    slider.prop_count = 1;
    
    doc.instances.add_instance(slider);
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_slider_create = output.find("lv_slider_create") != std::string_view::npos;
    
    return has_slider_create;
}

// Test 7: Type definitions as comments
constexpr bool test_type_definitions() {
    Document<4, 4, 4, 4, 4> doc;
    
    TypeDecl button_type;
    button_type.name = "MyButton";
    button_type.properties[0] = PropertyDecl{};
    button_type.properties[0].name = "label";
    button_type.properties[0].type = TypeRef{"string"};
    button_type.properties[1] = PropertyDecl{};
    button_type.properties[1].name = "enabled";
    button_type.properties[1].type = TypeRef{"bool"};
    button_type.prop_count = 2;
    
    doc.types[0] = button_type;
    doc.type_count = 1;
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_type_comment = output.find("/* Type Definitions */") != std::string_view::npos;
    bool has_my_button = output.find("MyButton") != std::string_view::npos;
    
    return has_type_comment && has_my_button;
}

// Test 8: Boolean property
constexpr bool test_boolean_property() {
    Document<4, 4, 4, 4, 4> doc;
    
    InstanceDecl checkbox;
    checkbox.type_name = "Checkbox";
    checkbox.properties[0] = PropertyAssignment{"checked", Value{Value::Kind::Bool, "true"}, Value{}, false};
    checkbox.prop_count = 1;
    
    doc.instances.add_instance(checkbox);
    
    LVGLRenderer<2048> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    bool has_checkbox = output.find("lv_checkbox_create") != std::string_view::npos;
    bool has_checked = output.find("lv_checkbox_set_checked") != std::string_view::npos;
    bool has_true = output.find("true") != std::string_view::npos;
    
    return has_checkbox && has_checked && has_true;
}

// Test 9: Event callbacks (when blocks)
constexpr bool test_event_callbacks() {
    Document<4, 4, 4, 4, 8> doc;
    
    // Create a Button with onClick event
    InstanceDecl button;
    button.type_name = "Button";
    button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click"}, Value{}, false};
    button.prop_count = 1;
    
    // Add when block
    WhenStmt when;
    when.condition = "clicked";
    when.assignments[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Clicked!"}, Value{}, false};
    when.assignment_count = 1;
    
    button.when_stmts[0] = when;
    button.when_count = 1;
    
    doc.instances.add_instance(button);
    
    LVGLRenderer<4096> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    // Verify callback function is generated
    bool has_callback = output.find("_callback_") != std::string_view::npos;
    bool has_lv_event = output.find("lv_event_t") != std::string_view::npos;
    bool has_add_event = output.find("lv_obj_add_event_cb") != std::string_view::npos;
    
    return has_callback && has_lv_event && has_add_event;
}

// Test 10: Animation support
constexpr bool test_animation() {
    Document<4, 4, 4, 4, 8> doc;
    
    // Create a Button with animation
    InstanceDecl button;
    button.type_name = "Button";
    button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Animate"}, Value{}, false};
    button.prop_count = 1;
    
    // Add animation
    AnimationDecl anim;
    anim.target_property = "x";
    anim.start_value = Value{Value::Kind::Integer, "0"};
    anim.end_value = Value{Value::Kind::Integer, "100"};
    anim.duration_ms = 500;
    anim.easing = "ease_in";
    anim.delay_ms = 0;
    anim.repeat = false;
    
    button.animations[0] = anim;
    button.animation_count = 1;
    
    doc.instances.add_instance(button);
    
    LVGLRenderer<4096> renderer;
    renderer.generate(doc);
    
    auto output = renderer.get_output();
    
    // Verify animation code is generated
    bool has_anim_t = output.find("lv_anim_t") != std::string_view::npos;
    bool has_anim_init = output.find("lv_anim_init") != std::string_view::npos;
    bool has_anim_set_values = output.find("lv_anim_set_values") != std::string_view::npos;
    bool has_anim_set_time = output.find("lv_anim_set_time") != std::string_view::npos;
    bool has_anim_start = output.find("lv_anim_start") != std::string_view::npos;
    
    return has_anim_t && has_anim_init && has_anim_set_values && has_anim_set_time && has_anim_start;
}

// Runtime test wrapper for all tests
inline void run_all_tests() {
    std::cout << "LVGL Renderer Tests\n";
    std::cout << "===================\n\n";
    
    std::cout << "Test 1: Simple button... ";
    assert(test_simple_button());
    std::cout << "✓\n";
    
    std::cout << "Test 2: Label... ";
    assert(test_label());
    std::cout << "✓\n";
    
    std::cout << "Test 3: Container with size... ";
    assert(test_container_with_size());
    std::cout << "✓\n";
    
    std::cout << "Test 4: Enum generation... ";
    assert(test_enum_generation());
    std::cout << "✓\n";
    
    std::cout << "Test 5: Multiple widgets... ";
    assert(test_multiple_widgets());
    std::cout << "✓\n";
    
    std::cout << "Test 6: Slider... ";
    assert(test_slider());
    std::cout << "✓\n";
    
    std::cout << "Test 7: Type definitions... ";
    assert(test_type_definitions());
    std::cout << "✓\n";
    
    std::cout << "Test 8: Boolean property... ";
    assert(test_boolean_property());
    std::cout << "✓\n";
    
    std::cout << "Test 9: Event callbacks (when blocks)... ";
    assert(test_event_callbacks());
    std::cout << "✓\n";
    
    std::cout << "Test 10: Animation support... ";
    assert(test_animation());
    std::cout << "✓\n";
    
    std::cout << "\n✓ All LVGL renderer tests passed!\n";
}

} // namespace forma::lvgl::tests
