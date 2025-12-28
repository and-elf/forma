// Forma Parser Demo
// Demonstrates parsing Forma source code into IR structures

#include "ir.hpp"
#include <iostream>
#include <iomanip>

void print_value(const Value& val) {
    std::cout << "\"" << val.text << "\" (";
    switch (val.kind) {
        case Value::Kind::Integer: std::cout << "int"; break;
        case Value::Kind::Float: std::cout << "float"; break;
        case Value::Kind::String: std::cout << "string"; break;
        case Value::Kind::Bool: std::cout << "bool"; break;
        case Value::Kind::Identifier: std::cout << "id"; break;
    }
    std::cout << ")";
}

void print_type_decl(const TypeDecl& decl) {
    std::cout << "\nType: " << decl.name << "\n";
    std::cout << "Properties (" << decl.prop_count << "):\n";
    for (size_t i = 0; i < decl.prop_count; ++i) {
        const auto& prop = decl.properties[i];
        std::cout << "  - " << prop.name << ": " << prop.type.name << "\n";
    }
}

void print_instance(const InstanceDecl& inst, const InstanceNode& storage, int indent = 0) {
    std::string ind(static_cast<size_t>(indent * 2), ' ');
    
    std::cout << ind << inst.type_name << " {\n";
    
    // Print properties
    for (size_t i = 0; i < inst.prop_count; ++i) {
        const auto& prop = inst.properties[i];
        std::cout << ind << "  " << prop.name << ": ";
        print_value(prop.value);
        std::cout << "\n";
    }
    
    // Print children
    for (size_t i = 0; i < inst.child_count; ++i) {
        const auto& child = storage.get(inst.child_start_idx + i);
        print_instance(child, storage, indent + 1);
    }
    
    std::cout << ind << "}\n";
}

int main() {
    std::cout << "=== Forma Parser Demo ===\n";
    std::cout << "========================\n\n";
    
    // Example 1: Type Declaration
    std::cout << "Example 1: Type Declaration\n";
    std::cout << "---------------------------\n";
    constexpr auto type_source = R"(Rectangle {
        property width: int
        property height: int
        property color: string
        property visible: bool
    })";
    
    std::cout << "Source:\n" << type_source << "\n";
    
    constexpr TypeDecl rect_type = parse_type_from_source(type_source);
    print_type_decl(rect_type);
    
    // Example 2: Simple Instance
    std::cout << "\n\nExample 2: Simple Instance\n";
    std::cout << "--------------------------\n";
    constexpr auto simple_source = R"(Rectangle {
        width: 100
        height: 50
        color: "blue"
        visible: true
    })";
    
    std::cout << "Source:\n" << simple_source << "\n\n";
    std::cout << "Parsed IR:\n";
    
    InstanceNode simple_storage;
    InstanceDecl simple_inst = parse_instance_with_storage(simple_source, simple_storage);
    print_instance(simple_inst, simple_storage);
    
    // Example 3: Nested Instance
    std::cout << "\n\nExample 3: Nested Instance\n";
    std::cout << "--------------------------\n";
    constexpr auto nested_source = R"(Window {
        title: "My Application"
        width: 800
        height: 600
        Column {
            spacing: 10
            Rectangle {
                width: 100
                height: 50
                color: "red"
            }
            Text {
                content: "Hello, Forma!"
                size: 14
            }
            Rectangle {
                width: 200
                height: 100
                color: "green"
            }
        }
    })";
    
    std::cout << "Source:\n" << nested_source << "\n\n";
    std::cout << "Parsed IR:\n";
    
    InstanceNode nested_storage;
    InstanceDecl nested_inst = parse_instance_with_storage(nested_source, nested_storage);
    print_instance(nested_inst, nested_storage);
    
    std::cout << "\nTotal instances in tree: " << nested_storage.count + 1 << "\n";
    
    // Example 4: Show compile-time parsing
    std::cout << "\n\nExample 4: Compile-time Parsing\n";
    std::cout << "--------------------------------\n";
    std::cout << "All parsing is done at compile time using constexpr!\n";
    std::cout << "The IR structures are evaluated during compilation.\n\n";
    
    constexpr auto demo_source = "Widget { enabled: true }";
    constexpr InstanceDecl demo = parse_instance_from_source(demo_source);
    
    std::cout << "Compile-time parsed: " << demo.type_name << "\n";
    std::cout << "Property count: " << demo.prop_count << "\n";
    static_assert(demo.type_name == "Widget", "Type name must be Widget");
    static_assert(demo.prop_count == 1, "Must have 1 property");
    std::cout << "✓ Static assertions passed!\n";
    
    return 0;
}
