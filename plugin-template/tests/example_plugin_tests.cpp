#include "example_plugin.hpp"
#include <iostream>
#include <cassert>

using namespace forma::example;

// Test 1: Basic functionality
constexpr bool test_basic() {
    Document<4, 4, 4, 4, 4> doc;
    
    EnumDecl color_enum;
    color_enum.name = "Color";
    color_enum.values[0] = "Red";
    color_enum.values[1] = "Green";
    color_enum.values[2] = "Blue";
    color_enum.value_count = 3;
    doc.enums[doc.enum_count++] = color_enum;
    
    ExamplePlugin<1024> plugin;
    plugin.process(doc);
    
    auto output = plugin.get_output();
    return output.find("enum Color") != std::string_view::npos;
}

int main() {
    std::cout << "Running Example Plugin Tests\n";
    std::cout << "============================\n\n";
    
    std::cout << "Test 1: Basic functionality... ";
    assert(test_basic());
    std::cout << "✓\n";
    
    std::cout << "\n✓ All tests passed!\n";
    return 0;
}
