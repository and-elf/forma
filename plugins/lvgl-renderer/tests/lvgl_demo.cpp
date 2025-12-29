#include "../src/lvgl_renderer.hpp"
#include <iostream>

using namespace forma;
using namespace forma::lvgl;

// Demo: Generate LVGL code for a complete UI
void demo_ui_generation() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Forma LVGL Renderer Demo\n";
    std::cout << "========================================\n\n";
    
    // Create a document with a UI layout
    Document<8, 4, 4, 4, 16> doc;
    
    // Add type definitions
    TypeDecl panel_type;
    panel_type.name = "ControlPanel";
    panel_type.properties[0].name = "width";
    panel_type.properties[0].type = TypeRef{"int"};
    panel_type.properties[1].name = "height";
    panel_type.properties[1].type = TypeRef{"int"};
    panel_type.prop_count = 2;
    doc.types[0] = panel_type;
    doc.type_count = 1;
    
    // Add an enum
    EnumDecl state_enum;
    state_enum.name = "State";
    state_enum.values[0] = EnumValue{"Idle"};
    state_enum.values[1] = EnumValue{"Running"};
    state_enum.values[2] = EnumValue{"Paused"};
    state_enum.values[3] = EnumValue{"Error"};
    state_enum.value_count = 4;
    doc.enums[0] = state_enum;
    doc.enum_count = 1;
    
    // Create a container panel
    InstanceDecl panel;
    panel.type_name = "Panel";
    panel.properties[0] = PropertyAssignment{"width", Value{Value::Kind::Integer, "320"}, Value{}, false};
    panel.properties[1] = PropertyAssignment{"height", Value{Value::Kind::Integer, "240"}, Value{}, false};
    panel.properties[2] = PropertyAssignment{"x", Value{Value::Kind::Integer, "0"}, Value{}, false};
    panel.properties[3] = PropertyAssignment{"y", Value{Value::Kind::Integer, "0"}, Value{}, false};
    panel.prop_count = 4;
    doc.instances.add_instance(panel);
    
    // Create a title label
    InstanceDecl title;
    title.type_name = "Label";
    title.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Device Control"}, Value{}, false};
    title.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    title.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "10"}, Value{}, false};
    title.prop_count = 3;
    doc.instances.add_instance(title);
    
    // Create a slider
    InstanceDecl brightness_slider;
    brightness_slider.type_name = "Slider";
    brightness_slider.properties[0] = PropertyAssignment{"value", Value{Value::Kind::Integer, "75"}, Value{}, false};
    brightness_slider.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    brightness_slider.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "50"}, Value{}, false};
    brightness_slider.properties[3] = PropertyAssignment{"width", Value{Value::Kind::Integer, "200"}, Value{}, false};
    brightness_slider.prop_count = 4;
    doc.instances.add_instance(brightness_slider);
    
    // Create a start button
    InstanceDecl start_btn;
    start_btn.type_name = "Button";
    start_btn.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Start"}, Value{}, false};
    start_btn.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    start_btn.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "100"}, Value{}, false};
    start_btn.properties[3] = PropertyAssignment{"width", Value{Value::Kind::Integer, "100"}, Value{}, false};
    start_btn.prop_count = 4;
    doc.instances.add_instance(start_btn);
    
    // Create a stop button
    InstanceDecl stop_btn;
    stop_btn.type_name = "Button";
    stop_btn.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Stop"}, Value{}, false};
    stop_btn.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "120"}, Value{}, false};
    stop_btn.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "100"}, Value{}, false};
    stop_btn.properties[3] = PropertyAssignment{"width", Value{Value::Kind::Integer, "100"}, Value{}, false};
    stop_btn.prop_count = 4;
    doc.instances.add_instance(stop_btn);
    
    // Create a status label
    InstanceDecl status;
    status.type_name = "Label";
    status.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Ready"}, Value{}, false};
    status.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    status.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "150"}, Value{}, false};
    status.prop_count = 3;
    doc.instances.add_instance(status);
    
    // Create a switch
    InstanceDecl enable_switch;
    enable_switch.type_name = "Switch";
    enable_switch.properties[0] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    enable_switch.properties[1] = PropertyAssignment{"y", Value{Value::Kind::Integer, "180"}, Value{}, false};
    enable_switch.prop_count = 2;
    doc.instances.add_instance(enable_switch);
    
    // Create a checkbox
    InstanceDecl auto_checkbox;
    auto_checkbox.type_name = "Checkbox";
    auto_checkbox.properties[0] = PropertyAssignment{"checked", Value{Value::Kind::Bool, "false"}, Value{}, false};
    auto_checkbox.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "100"}, Value{}, false};
    auto_checkbox.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "180"}, Value{}, false};
    auto_checkbox.prop_count = 3;
    doc.instances.add_instance(auto_checkbox);
    
    // Generate the LVGL C code
    LVGLRenderer<16384> renderer;
    renderer.generate(doc);
    
    // Output the generated code
    std::cout << "Generated C99 LVGL Code:\n";
    std::cout << "------------------------\n\n";
    std::cout << renderer.c_str() << "\n";
}

int main() {
    // Run the demo
    demo_ui_generation();
    
    return 0;
}
