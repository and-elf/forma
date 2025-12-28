#include "example_plugin.hpp"
#include <iostream>

using namespace forma::example;

int main() {
    std::cout << "Forma Example Plugin Demo\n";
    std::cout << "=========================\n\n";
    
    // Create a simple document
    Document<4, 4, 4, 4, 4> doc;
    
    // Add an enum
    EnumDecl status;
    status.name = "Status";
    status.values[0] = "Active";
    status.values[1] = "Inactive";
    status.values[2] = "Error";
    status.value_count = 3;
    doc.enums[doc.enum_count++] = status;
    
    // Process with plugin
    ExamplePlugin<2048> plugin;
    plugin.process(doc);
    
    // Output result
    std::cout << "Generated Output:\n";
    std::cout << "-----------------\n";
    std::cout << plugin.c_str() << "\n";
    
    return 0;
}
