#include <bugspray/bugspray.hpp>
#include "../src/lvgl_renderer.hpp"

using namespace forma;
using namespace forma::lvgl;

TEST_CASE("LVGL - Basic Widget Creation")
{
    SECTION("Simple button with text")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        InstanceDecl button;
        button.type_name = "Button";
        button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click Me"}, Value{}, false};
        button.prop_count = 1;
        
        doc.instances.add_instance(button);
        
        LVGLRenderer<2048> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        CHECK(output.find("#include \"lvgl.h\"") != std::string_view::npos);
        CHECK(output.find("lv_btn_create") != std::string_view::npos);
        CHECK(output.find("void forma_init(void)") != std::string_view::npos);
    }
    
    SECTION("Label with text")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        InstanceDecl label;
        label.type_name = "Label";
        label.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Hello LVGL"}, Value{}, false};
        label.prop_count = 1;
        
        doc.instances.add_instance(label);
        
        LVGLRenderer<2048> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        CHECK(output.find("lv_label_create") != std::string_view::npos);
        CHECK(output.find("lv_label_set_text") != std::string_view::npos);
    }
    
    SECTION("Slider widget")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        InstanceDecl slider;
        slider.type_name = "Slider";
        slider.properties[0] = PropertyAssignment{"value", Value{Value::Kind::Integer, "50"}, Value{}, false};
        slider.prop_count = 1;
        
        doc.instances.add_instance(slider);
        
        LVGLRenderer<2048> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        CHECK(output.find("lv_slider_create") != std::string_view::npos);
    }
    
    SECTION("Checkbox with boolean")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        InstanceDecl checkbox;
        checkbox.type_name = "Checkbox";
        checkbox.properties[0] = PropertyAssignment{"checked", Value{Value::Kind::Bool, "true"}, Value{}, false};
        checkbox.prop_count = 1;
        
        doc.instances.add_instance(checkbox);
        
        LVGLRenderer<2048> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        CHECK(output.find("lv_checkbox_create") != std::string_view::npos);
        CHECK(output.find("lv_checkbox_set_checked") != std::string_view::npos);
        CHECK(output.find("true") != std::string_view::npos);
    }
}

TEST_CASE("LVGL - Container and Sizing")
{
    SECTION("Container with width and height")
    {
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
        
        CHECK(output.find("lv_obj_create") != std::string_view::npos);
        CHECK(output.find("lv_obj_set_width") != std::string_view::npos);
        CHECK(output.find("lv_obj_set_height") != std::string_view::npos);
    }
    
    SECTION("Multiple widgets with positioning")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        InstanceDecl button;
        button.type_name = "Button";
        button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "OK"}, Value{}, false};
        button.properties[1] = PropertyAssignment{"x", Value{Value::Kind::Integer, "10"}, Value{}, false};
        button.properties[2] = PropertyAssignment{"y", Value{Value::Kind::Integer, "20"}, Value{}, false};
        button.prop_count = 3;
        
        doc.instances.add_instance(button);
        
        LVGLRenderer<4096> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        CHECK(output.find("lv_btn_create") != std::string_view::npos);
        CHECK(output.find("lv_obj_set_x") != std::string_view::npos);
        CHECK(output.find("lv_obj_set_y") != std::string_view::npos);
    }
}

TEST_CASE("LVGL - Type System")
{
    SECTION("Enum generation")
    {
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
        
        CHECK(output.find("typedef enum") != std::string_view::npos);
        CHECK(output.find("Color_Red") != std::string_view::npos);
        CHECK(output.find("Color_Green") != std::string_view::npos);
        CHECK(output.find("Color_Blue") != std::string_view::npos);
    }
    
    SECTION("Type definitions as comments")
    {
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
        
        CHECK(output.find("/* Type Definitions */") != std::string_view::npos);
        CHECK(output.find("MyButton") != std::string_view::npos);
    }
}

TEST_CASE("LVGL - Event Callbacks")
{
    SECTION("When block generates callback")
    {
        Document<4, 4, 4, 4, 8> doc;
        
        InstanceDecl button;
        button.type_name = "Button";
        button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click"}, Value{}, false};
        button.prop_count = 1;
        
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
        
        CHECK(output.find("_callback_") != std::string_view::npos);
        CHECK(output.find("lv_event_t") != std::string_view::npos);
        CHECK(output.find("lv_obj_add_event_cb") != std::string_view::npos);
    }
}

TEST_CASE("LVGL - Animations")
{
    SECTION("Button with slide animation")
    {
        Document<4, 4, 4, 4, 8> doc;
        
        InstanceDecl button;
        button.type_name = "Button";
        button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Animate"}, Value{}, false};
        button.prop_count = 1;
        
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
        
        CHECK(output.find("lv_anim_t") != std::string_view::npos);
        CHECK(output.find("lv_anim_init") != std::string_view::npos);
        CHECK(output.find("lv_anim_set_values") != std::string_view::npos);
        CHECK(output.find("lv_anim_set_time") != std::string_view::npos);
        CHECK(output.find("lv_anim_start") != std::string_view::npos);
    }
}

TEST_CASE("LVGL - Class Generation")
{
    SECTION("Class with properties")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        // Define a Counter class
        TypeDecl counter_class;
        counter_class.name = "Counter";
        PropertyDecl value_prop;
        value_prop.name = "value";
        value_prop.type = TypeRef{"int"};
        value_prop.reactive = false;
        counter_class.properties[0] = value_prop;
        counter_class.prop_count = 1;
        
        // Add a method
        MethodDecl increment_method;
        increment_method.name = "increment";
        increment_method.return_type = TypeRef{"void"};
        increment_method.param_count = 0;
        counter_class.methods[0] = increment_method;
        counter_class.method_count = 1;
        
        doc.types[0] = counter_class;
        doc.type_count = 1;
        
        // Add a button widget
        InstanceDecl button;
        button.type_name = "Button";
        button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click Me"}, Value{}, false};
        button.prop_count = 1;
        doc.instances.add_instance(button);
        
        LVGLRenderer<4096> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        // Check class struct definition
        CHECK(output.find("typedef struct {") != std::string_view::npos);
        CHECK(output.find("int32_t value;") != std::string_view::npos);
        CHECK(output.find("} Counter;") != std::string_view::npos);
        
        // Check global instance
        CHECK(output.find("Counter counter") != std::string_view::npos);
        CHECK(output.find(".value = 0") != std::string_view::npos);
        
        // Should NOT contain method declarations (user implements them)
        CHECK(output.find("void Counter_increment") == std::string_view::npos);
    }
    
    SECTION("Class with multiple methods and parameters")
    {
        Document<4, 4, 4, 4, 4> doc;
        
        // Define a Calculator class
        TypeDecl calc_class;
        calc_class.name = "Calculator";
        PropertyDecl result_prop;
        result_prop.name = "result";
        result_prop.type = TypeRef{"int"};
        result_prop.reactive = false;
        calc_class.properties[0] = result_prop;
        calc_class.prop_count = 1;
        
        // Add method
        MethodDecl add_method;
        add_method.name = "add";
        add_method.return_type = TypeRef{"void"};
        add_method.params[0] = MethodParam{"a", TypeRef{"int"}};
        add_method.params[1] = MethodParam{"b", TypeRef{"int"}};
        add_method.param_count = 2;
        calc_class.methods[0] = add_method;
        calc_class.method_count = 1;
        
        doc.types[0] = calc_class;
        doc.type_count = 1;
        
        LVGLRenderer<4096> renderer;
        renderer.generate(doc);
        
        auto output = renderer.get_output();
        
        // Check method with parameters - should NOT be generated
        CHECK(output.find("void Calculator_add") == std::string_view::npos);
        CHECK(output.find("Calculator calculator") != std::string_view::npos);
    }
}
