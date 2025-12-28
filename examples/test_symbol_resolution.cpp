#include "src/lsp.hpp"
#include <iostream>

int main() {
    using namespace forma::lsp;
    
    LSPDocumentManager<16> manager;
    
    // Test 1: Forward reference - Point is used before it's defined
    const char* code1 = R"(
MyRect {
    property position: Point
}

Point {
    property x: int
    property y: int
}
)";
    
    TextDocumentItem item1;
    item1.uri = "file:///test1.fml";
    item1.text = code1;
    item1.version = 1;
    
    manager.did_open(item1);
    
    auto* doc1 = manager.find_document("file:///test1.fml");
    std::cout << "Test 1 - Forward Reference:\n";
    std::cout << "  Diagnostics: " << doc1->diagnostic_count << "\n";
    for (size_t i = 0; i < doc1->diagnostic_count; ++i) {
        auto& diag = doc1->diagnostics[i];
        std::cout << "  - " << diag.message << " (code: " << diag.code << ")\n";
    }
    
    // Test 2: Unknown type reference
    const char* code2 = R"(
MyRect {
    property position: UnknownType
}
)";
    
    TextDocumentItem item2;
    item2.uri = "file:///test2.fml";
    item2.text = code2;
    item2.version = 1;
    
    manager.did_open(item2);
    
    auto* doc2 = manager.find_document("file:///test2.fml");
    std::cout << "\nTest 2 - Unknown Type:\n";
    std::cout << "  Diagnostics: " << doc2->diagnostic_count << "\n";
    for (size_t i = 0; i < doc2->diagnostic_count; ++i) {
        auto& diag = doc2->diagnostics[i];
        std::cout << "  - " << diag.message << " (code: " << diag.code << ")\n";
    }
    
    // Test 3: Valid references
    const char* code3 = R"(
Point {
    property x: int
    property y: int
}

MyRect {
    property position: Point
}
)";
    
    TextDocumentItem item3;
    item3.uri = "file:///test3.fml";
    item3.text = code3;
    item3.version = 1;
    
    manager.did_open(item3);
    
    auto* doc3 = manager.find_document("file:///test3.fml");
    std::cout << "\nTest 3 - Valid References:\n";
    std::cout << "  Diagnostics: " << doc3->diagnostic_count << "\n";
    for (size_t i = 0; i < doc3->diagnostic_count; ++i) {
        auto& diag = doc3->diagnostics[i];
        std::cout << "  - " << diag.message << " (code: " << diag.code << ")\n";
    }
    
    return 0;
}
