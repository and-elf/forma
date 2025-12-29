#include <bugspray/bugspray.hpp>
#include "ir.hpp"

using namespace forma;

TEST_CASE("Parser - Property Declaration")
{
    Parser p("property width: int");
    PropertyDecl decl = parse_property(p);
    
    CHECK(decl.name == "width");
    CHECK(decl.type.name == "int");
}

TEST_CASE("Parser - Type Declaration")
{
    SECTION("Single property")
    {
        constexpr auto source = R"(Rectangle {
            property width: int
        })";
        
        TypeDecl decl = parse_type_from_source(source);
        
        CHECK(decl.name == "Rectangle");
        CHECK(decl.prop_count == 1ul);
        CHECK(decl.properties[0].name == "width");
        CHECK(decl.properties[0].type.name == "int");
    }
    
    SECTION("Multiple properties")
    {
        constexpr auto source = R"(Rectangle {
            property width: int
            property height: int
            property color: string
        })";
        
        TypeDecl decl = parse_type_from_source(source);
        
        CHECK(decl.name == "Rectangle");
        CHECK(decl.prop_count == 3ul);
        CHECK(decl.properties[0].name == "width");
        CHECK(decl.properties[1].name == "height");
        CHECK(decl.properties[2].name == "color");
    }
}

TEST_CASE("Parser - Instance Declaration")
{
    SECTION("Integer property")
    {
        constexpr auto source = R"(Rectangle {
            width: 100
        })";
        
        InstanceDecl inst = parse_instance_from_source(source);
        
        CHECK(inst.type_name == "Rectangle");
        CHECK(inst.prop_count == 1ul);
        CHECK(inst.properties[0].name == "width");
        CHECK(inst.properties[0].value.kind == Value::Kind::Integer);
        CHECK(inst.properties[0].value.text == "100");
    }
    
    SECTION("String property")
    {
        constexpr auto source = R"(Text {
            content: "Hello, World"
        })";
        
        InstanceDecl inst = parse_instance_from_source(source);
        
        CHECK(inst.type_name == "Text");
        CHECK(inst.prop_count == 1ul);
        CHECK(inst.properties[0].name == "content");
        CHECK(inst.properties[0].value.kind == Value::Kind::String);
        CHECK(inst.properties[0].value.text == "Hello, World");
    }
    
    SECTION("Boolean property")
    {
        constexpr auto source = R"(Widget {
            visible: true
        })";
        
        InstanceDecl inst = parse_instance_from_source(source);
        
        CHECK(inst.type_name == "Widget");
        CHECK(inst.prop_count == 1ul);
        CHECK(inst.properties[0].name == "visible");
        CHECK(inst.properties[0].value.kind == Value::Kind::Bool);
        CHECK(inst.properties[0].value.text == "true");
    }
    
    SECTION("Multiple properties")
    {
        constexpr auto source = R"(Rectangle {
            width: 100
            height: 200
            visible: true
        })";
        
        InstanceDecl inst = parse_instance_from_source(source);
        
        CHECK(inst.type_name == "Rectangle");
        CHECK(inst.prop_count == 3ul);
        CHECK(inst.properties[0].name == "width");
        CHECK(inst.properties[1].name == "height");
        CHECK(inst.properties[2].name == "visible");
    }
}

TEST_CASE("Parser - Nested Instances")
{
    SECTION("Simple nesting")
    {
        constexpr auto source = R"(Container {
            child: Rectangle {
                width: 100
            }
        })";
        
        InstanceDecl inst = parse_instance_from_source(source);
        
        CHECK(inst.type_name == "Container");
        CHECK(inst.prop_count == 1ul);
        CHECK(inst.properties[0].name == "child");
        // Note: Nested instances are represented differently in current IR
    }
}

TEST_CASE("Parser - Enum Declaration")
{
    constexpr auto source = R"(enum Alignment {
        Left,
        Center,
        Right
    })";
    
    EnumDecl decl = parse_enum_from_source(source);
    
    CHECK(decl.name == "Alignment");
    CHECK(decl.value_count == 3ul);
    CHECK(decl.values[0].name == "Left");
    CHECK(decl.values[1].name == "Center");
    CHECK(decl.values[2].name == "Right");
}

TEST_CASE("Parser - Document Parsing")
{
    SECTION("Multiple component types")
    {
        constexpr auto type_source = R"(Rectangle {
            property width: int
            property height: int
        })";
        
        constexpr auto event_source = "event onSizeChanged(width: int, height: int)";
        
        TypeDecl type = parse_type_from_source(type_source);
        EventDecl event = parse_event_from_source(event_source);
        
        CHECK(type.name == "Rectangle");
        CHECK(type.prop_count == 2ul);
        CHECK(event.name == "onSizeChanged");
        CHECK(event.param_count == 2ul);
    }
    
    SECTION("Import statements")
    {
        constexpr auto source = R"(
import forma.animation
import forma.color

enum Status {
    Active,
    Inactive
}

class Button {
    property string text
}

Button {
    text: "Click"
}
)";
        
        constexpr auto doc = parse_document(source);
        
        CHECK(doc.import_count == 2ul);
        CHECK(doc.imports[0].module_path == "forma.animation");
        CHECK(doc.imports[1].module_path == "forma.color");
        CHECK(doc.enum_count == 1ul);
        CHECK(doc.enums[0].name == "Status");
        CHECK(doc.type_count == 1ul);
        CHECK(doc.types[0].name == "Button");
        CHECK(doc.instances.count == 1ul);
    }
}
