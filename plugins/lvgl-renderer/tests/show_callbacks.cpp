#include "lvgl_tests.hpp"
#include <iostream>

using namespace forma;
using namespace forma::lvgl;

int main() {
    Document<4, 4, 4, 4, 8> doc;
    
    // Create a Button with onClick event
    InstanceDecl button;
    button.type_name = "Button";
    button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click Me"}, Value{}, false};
    button.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
    button.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "20"}, Value{}, false};
    button.prop_count = 3;
    
    // Add when block for onClick
    WhenStmt when_click;
    when_click.condition = "clicked";
    when_click.assignments[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Clicked!"}, Value{}, false};
    when_click.assignment_count = 1;
    
    button.when_stmts[0] = when_click;
    button.when_count = 1;
    
    doc.instances.add_instance(button);
    
    LVGLRenderer<8192> renderer;
    renderer.generate(doc);
    
    std::cout << renderer.get_output() << std::endl;
    
    return 0;
}
