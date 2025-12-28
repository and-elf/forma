#include "src/parser/ir.hpp"
#include "plugins/lvgl-renderer/src/lvgl_renderer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace forma;
using namespace forma::lvgl;

std::string read_file(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.forma>\n";
        return 1;
    }
    
    std::string source = read_file(argv[1]);
    std::cout << "Parsing: " << argv[1] << "\n";
    std::cout << "Source:\n" << source << "\n\n";
    
    // Parse the document (this automatically handles nested instances)
    auto doc = parse_document(source);
    
    std::cout << "Parsed " << doc.instances.count << " total instances (including children)\n";
    for (size_t i = 0; i < doc.instances.count; ++i) {
        const auto& inst = doc.instances.get(i);
        std::cout << "  Instance " << i << ": " << std::string_view(inst.type_name.data(), inst.type_name.size())
                  << " (children: " << inst.child_count << ")\n";
    }
    std::cout << "\n";
    
    // Generate LVGL code
    LVGLRenderer renderer;
    renderer.generate(doc);
    
    std::cout << "Generated LVGL Code:\n";
    std::cout << "====================\n";
    std::cout << renderer.get_output() << "\n";
    
    return 0;
}
