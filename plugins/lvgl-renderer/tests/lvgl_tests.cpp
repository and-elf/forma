#include "../src/lvgl_renderer.hpp"
#include "lvgl_tests.hpp"
#include <iostream>

using namespace forma;
using namespace forma::lvgl;

int main() {
    forma::lvgl::tests::run_all_tests();
    
    std::cout << "\n========================================\n";
    std::cout << "Example Generated Code with Animation\n";
    std::cout << "========================================\n\n";
    
    // Show generated code for button with animation
    Document<4, 4, 4, 4, 8> doc;
    
    InstanceDecl button;
    button.type_name = "Button";
    button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Slide Me"}, Value{}, false};
    button.properties[1] = PropertyAssignment{"y", Value{Value::Kind::Integer, "50"}, Value{}, false};
    button.prop_count = 2;
    
    // Add slide animation
    AnimationDecl anim;
    anim.target_property = "x";
    anim.start_value = Value{Value::Kind::Integer, "0"};
    anim.end_value = Value{Value::Kind::Integer, "200"};
    anim.duration_ms = 1000;
    anim.easing = "ease_in_out";
    anim.delay_ms = 500;
    anim.repeat = true;
    
    button.animations[0] = anim;
    button.animation_count = 1;
    
    doc.instances.add_instance(button);
    
    LVGLRenderer<8192> renderer;
    renderer.generate(doc);
    
    std::cout << renderer.get_output() << "\n";
    
    return 0;
}

