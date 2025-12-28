#include "plugins/lvgl-renderer/src/lvgl_renderer.hpp"
#include "src/forma.hpp"
#include <iostream>

using namespace forma;
using namespace forma::lvgl;

int main() {
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
    
    std::cout << renderer.get_output() << std::endl;
    
    return 0;
}
