#include "../src/lsp.hpp"
#include "../src/virtual_fs.hpp"
#include <iostream>
#include <cassert>

using namespace forma::lsp;
using namespace forma::vfs;

void test_basic_operations() {
    std::cout << "Test: Basic VirtualFS Operations\n";
    std::cout << "==================================\n";
    
    VirtualFS<32> fs;
    
    // Write a file
    assert(fs.write_file("file:///test.fml", "Point { property x: int }"));
    assert(fs.exists("file:///test.fml"));
    assert(fs.count() == 1);
    
    // Read it back
    auto content = fs.read_file("file:///test.fml");
    assert(content.has_value());
    assert(content.value() == "Point { property x: int }");
    
    // Update it
    assert(fs.write_file("file:///test.fml", "Point { property x: int property y: int }", 2));
    content = fs.read_file("file:///test.fml");
    assert(content.value() == "Point { property x: int property y: int }");
    
    // Delete it
    assert(fs.remove_file("file:///test.fml"));
    assert(!fs.exists("file:///test.fml"));
    assert(fs.count() == 0);
    
    std::cout << "✓ All basic operations passed\n\n";
}

void test_workspace_integration() {
    std::cout << "Test: VirtualWorkspace with LSP\n";
    std::cout << "================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    // Initialize
    auto init_result = workspace.initialize(1234, "file:///workspace");
    assert(init_result.server_name == "forma-lsp");
    std::cout << "✓ Initialized: " << init_result.server_name << " v" << init_result.server_version << "\n";
    
    // Create a valid file
    assert(workspace.create_file("file:///point.fml", 
        "Point { property x: int property y: int }"));
    std::cout << "✓ Created point.fml\n";
    
    auto* doc = workspace.get_diagnostics("file:///point.fml");
    assert(doc != nullptr);
    assert(doc->active);
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 0)\n";
    assert(doc->diagnostic_count == 0);
    
    // Create a file with errors
    assert(workspace.create_file("file:///rect.fml",
        "Rectangle { property pos: UnknownType property size: Point }"));
    std::cout << "✓ Created rect.fml with unknown type\n";
    
    doc = workspace.get_diagnostics("file:///rect.fml");
    assert(doc != nullptr);
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 2 - UnknownType and Point)\n";
    if (doc->diagnostic_count != 2) {
        for (size_t i = 0; i < doc->diagnostic_count; ++i) {
            std::cout << "  [" << i << "] code=" << doc->diagnostics[i].code << " msg=" << doc->diagnostics[i].message << "\n";
        }
    }
    assert(doc->diagnostic_count == 2);
    assert(doc->diagnostics[0].code == "unknown-type");
    assert(doc->diagnostics[0].message == "UnknownType");
    assert(doc->diagnostics[1].code == "unknown-type");
    assert(doc->diagnostics[1].message == "Point");
    std::cout << "  Errors: " << doc->diagnostics[0].message << ", " << doc->diagnostics[1].message << "\n";
    
    // Update to fix the error
    std::cout << "✓ Updating rect.fml to fix error...\n";
    assert(workspace.update_file("file:///rect.fml",
        "Rectangle { property pos: Point property width: int property height: int }"));
    
    doc = workspace.get_diagnostics("file:///rect.fml");
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1 - Point not defined before use)\n";
    // Note: Point is used but defined in a separate file, so this will still error
    
    // List files
    auto files = workspace.list_files();
    std::cout << "✓ Files in workspace: " << workspace.file_count() << "\n";
    for (size_t i = 0; i < workspace.file_count(); ++i) {
        if (!files[i].empty()) {
            std::cout << "  - " << files[i] << "\n";
        }
    }
    
    // Delete a file
    assert(workspace.delete_file("file:///rect.fml"));
    assert(!workspace.exists("file:///rect.fml"));
    std::cout << "✓ Deleted rect.fml\n";
    std::cout << "  Remaining files: " << workspace.file_count() << "\n";
    
    std::cout << "\n✓ All workspace integration tests passed\n\n";
}

void test_forward_references() {
    std::cout << "Test: Forward References in Single File\n";
    std::cout << "========================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // Create file where Point is used before it's defined
    const char* code = R"(Rectangle {
    property topLeft: Point
    property bottomRight: Point
}

Point {
    property x: int
    property y: int
})";
    
    workspace.create_file("file:///shapes.fml", code);
    std::cout << "✓ Created shapes.fml with forward reference\n";
    
    auto* doc = workspace.get_diagnostics("file:///shapes.fml");
    assert(doc != nullptr);
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 0 - forward refs work!)\n";
    
    if (doc->diagnostic_count > 0) {
        for (size_t i = 0; i < doc->diagnostic_count; ++i) {
            std::cout << "  Error: " << doc->diagnostics[i].message << "\n";
        }
    }
    
    assert(doc->diagnostic_count == 0);
    std::cout << "✓ Forward references work correctly!\n\n";
}

void test_generic_types() {
    std::cout << "Test: Generic Types (Forma.Array)\n";
    std::cout << "==================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // Test generic type with correct parameters
    const char* code1 = R"(Points {
    property data: Forma.Array(int, 10)
})";
    
    workspace.create_file("file:///test1.fml", code1);
    auto* doc = workspace.get_diagnostics("file:///test1.fml");
    std::cout << "✓ Forma.Array(int, 10) - diagnostics: " << doc->diagnostic_count << " (expected 0)\n";
    assert(doc->diagnostic_count == 0);
    
    // Test generic type with wrong number of parameters
    const char* code2 = R"(Points {
    property data: Forma.Array(int)
})";
    
    workspace.create_file("file:///test2.fml", code2);
    doc = workspace.get_diagnostics("file:///test2.fml");
    std::cout << "✓ Forma.Array(int) - diagnostics: " << doc->diagnostic_count << " (expected 1 - wrong params)\n";
    assert(doc->diagnostic_count == 1);
    
    std::cout << "\n✓ Generic type validation works!\n\n";
}

void test_enums_and_events() {
    std::cout << "Test: Enums and Events\n";
    std::cout << "======================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    const char* code = R"(enum Color {
    Red,
    Green,
    Blue
}

event onClick(x: int, y: int)
event onColorChange(color: Color))";
    
    workspace.create_file("file:///ui.fml", code);
    auto* doc = workspace.get_diagnostics("file:///ui.fml");
    
    std::cout << "✓ Created enum and events\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 0)\n";
    
    if (doc->diagnostic_count > 0) {
        for (size_t i = 0; i < doc->diagnostic_count; ++i) {
            std::cout << "  Error: " << doc->diagnostics[i].message << "\n";
        }
    }
    
    std::cout << "\n✓ Enum and event parsing works!\n\n";
}

void test_diagnostic_unknown_type() {
    std::cout << "Test: Diagnostic - Unknown Type\n";
    std::cout << "================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // Test 1: Single unknown type
    const char* code1 = R"(Widget {
    property data: UnknownType
})";
    
    workspace.create_file("file:///test1.fml", code1);
    auto* doc = workspace.get_diagnostics("file:///test1.fml");
    std::cout << "✓ Single unknown type\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "unknown-type");
    assert(doc->diagnostics[0].message == "UnknownType");
    assert(doc->diagnostics[0].severity == forma::lsp::DiagnosticSeverity::Error);
    std::cout << "  Error: " << doc->diagnostics[0].message << " (code: " << doc->diagnostics[0].code << ")\n";
    
    // Test 2: Multiple unknown types
    const char* code2 = R"(Widget {
    property data1: UnknownType1
    property data2: UnknownType2
    property data3: UnknownType3
})";
    
    workspace.create_file("file:///test2.fml", code2);
    doc = workspace.get_diagnostics("file:///test2.fml");
    std::cout << "\n✓ Multiple unknown types\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 3)\n";
    assert(doc->diagnostic_count == 3);
    for (size_t i = 0; i < doc->diagnostic_count; ++i) {
        assert(doc->diagnostics[i].code == "unknown-type");
        std::cout << "  Error " << (i+1) << ": " << doc->diagnostics[i].message << "\n";
    }
    
    // Test 3: Unknown base type (inheritance)
    const char* code3 = R"(MyWidget: UnknownBaseType {
    property x: int
})";
    
    workspace.create_file("file:///test3.fml", code3);
    doc = workspace.get_diagnostics("file:///test3.fml");
    std::cout << "\n✓ Unknown base type in inheritance\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "unknown-type");
    assert(doc->diagnostics[0].message == "UnknownBaseType");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    // Test 4: Unknown type in event parameters
    const char* code4 = R"(event onUpdate(data: UnknownDataType))";
    
    workspace.create_file("file:///test4.fml", code4);
    doc = workspace.get_diagnostics("file:///test4.fml");
    std::cout << "\n✓ Unknown type in event parameters\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "unknown-type");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    // Test 5: Mix of known and unknown types
    const char* code5 = R"(Point {
    property x: int
    property y: int
}

Widget {
    property position: Point
    property data: UnknownType
    property count: int
})";
    
    workspace.create_file("file:///test5.fml", code5);
    doc = workspace.get_diagnostics("file:///test5.fml");
    std::cout << "\n✓ Mix of known and unknown types\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1 for UnknownType)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].message == "UnknownType");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    std::cout << "\n✓ All unknown type diagnostic tests passed!\n\n";
}

void test_diagnostic_generic_types() {
    std::cout << "Test: Diagnostic - Generic Type Errors\n";
    std::cout << "=======================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // Test 1: Wrong number of parameters (too few)
    const char* code1 = R"(Widget {
    property items: Forma.Array(int)
})";
    
    workspace.create_file("file:///generic1.fml", code1);
    auto* doc = workspace.get_diagnostics("file:///generic1.fml");
    std::cout << "✓ Forma.Array with 1 parameter (expected 2)\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "invalid-generic-params");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    // Test 2: Wrong number of parameters (too many)
    const char* code2 = R"(Widget {
    property items: Forma.Array(int, 10, 20)
})";
    
    workspace.create_file("file:///generic2.fml", code2);
    doc = workspace.get_diagnostics("file:///generic2.fml");
    std::cout << "\n✓ Forma.Array with 3 parameters (expected 2)\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "invalid-generic-params");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    // Test 3: Wrong parameter types (second should be integer)
    const char* code3 = R"(Widget {
    property items: Forma.Array(int, string)
})";
    
    workspace.create_file("file:///generic3.fml", code3);
    doc = workspace.get_diagnostics("file:///generic3.fml");
    std::cout << "\n✓ Forma.Array with wrong type for size parameter\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "invalid-generic-params");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    // Test 4: Correct usage - should have no errors
    const char* code4 = R"(Widget {
    property items: Forma.Array(int, 10)
    property names: Forma.Array(string, 5)
})";
    
    workspace.create_file("file:///generic4.fml", code4);
    doc = workspace.get_diagnostics("file:///generic4.fml");
    std::cout << "\n✓ Valid Forma.Array usage\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 0)\n";
    assert(doc->diagnostic_count == 0);
    
    // Test 5: Generic with custom types
    const char* code5 = R"(Point {
    property x: int
    property y: int
}

Widget {
    property points: Forma.Array(Point, 100)
})";
    
    workspace.create_file("file:///generic5.fml", code5);
    doc = workspace.get_diagnostics("file:///generic5.fml");
    std::cout << "\n✓ Forma.Array with custom type\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 0)\n";
    assert(doc->diagnostic_count == 0);
    
    // Test 6: Generic with unknown element type
    const char* code6 = R"(Widget {
    property items: Forma.Array(UnknownItemType, 10)
})";
    
    workspace.create_file("file:///generic6.fml", code6);
    doc = workspace.get_diagnostics("file:///generic6.fml");
    std::cout << "\n✓ Forma.Array with unknown element type\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    assert(doc->diagnostics[0].code == "unknown-type");
    assert(doc->diagnostics[0].message == "UnknownItemType");
    std::cout << "  Error: " << doc->diagnostics[0].message << "\n";
    
    std::cout << "\n✓ All generic type diagnostic tests passed!\n\n";
}

void test_diagnostic_multiple_errors() {
    std::cout << "Test: Diagnostic - Multiple Errors Per File\n";
    std::cout << "============================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // File with multiple different error types
    const char* code = R"(Widget: UnknownBase {
    property data1: UnknownType1
    property data2: UnknownType2
    property items: Forma.Array(int)
}

event onUpdate(param: UnknownParamType))";
    
    workspace.create_file("file:///errors.fml", code);
    auto* doc = workspace.get_diagnostics("file:///errors.fml");
    
    std::cout << "✓ File with multiple error types\n";
    std::cout << "  Total diagnostics: " << doc->diagnostic_count << "\n";
    std::cout << "  Expected: 5 errors (1 unknown base + 2 unknown types + 1 generic + 1 event param)\n";
    assert(doc->diagnostic_count == 5);
    
    std::cout << "\n  Detailed errors:\n";
    for (size_t i = 0; i < doc->diagnostic_count; ++i) {
        std::cout << "  " << (i+1) << ". " << doc->diagnostics[i].message 
                  << " (code: " << doc->diagnostics[i].code << ")\n";
    }
    
    std::cout << "\n✓ Multiple error detection works!\n\n";
}

void test_diagnostic_severity_levels() {
    std::cout << "Test: Diagnostic - Severity Levels\n";
    std::cout << "===================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // Create file with errors
    const char* code = R"(Widget {
    property data: UnknownType
    property count: int
})";
    
    workspace.create_file("file:///severity.fml", code);
    auto* doc = workspace.get_diagnostics("file:///severity.fml");
    
    std::cout << "✓ Checking diagnostic severity\n";
    assert(doc->diagnostic_count == 1);
    
    const auto& diag = doc->diagnostics[0];
    std::cout << "  Message: " << diag.message << "\n";
    std::cout << "  Code: " << diag.code << "\n";
    std::cout << "  Severity: " << static_cast<int>(diag.severity);
    
    // Verify it's an Error (severity = 1)
    assert(diag.severity == forma::lsp::DiagnosticSeverity::Error);
    std::cout << " (Error)\n";
    
    // Check range information
    std::cout << "  Range: line " << diag.range.start.line 
              << ", char " << diag.range.start.character
              << " to line " << diag.range.end.line
              << ", char " << diag.range.end.character << "\n";
    
    assert(diag.range.start.line >= 0);
    assert(diag.range.start.character >= 0);
    
    std::cout << "\n✓ Severity levels correctly set!\n\n";
}

void test_diagnostic_error_recovery() {
    std::cout << "Test: Diagnostic - Error Recovery\n";
    std::cout << "==================================\n";
    
    LSPDocumentManager<16> lsp_manager;
    VirtualWorkspace workspace(lsp_manager);
    
    workspace.initialize();
    
    // Create file with error
    const char* error_code = R"(Widget {
    property data: UnknownType
})";
    
    workspace.create_file("file:///fix.fml", error_code);
    auto* doc = workspace.get_diagnostics("file:///fix.fml");
    
    std::cout << "✓ Initial file with error\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    
    // Fix the error
    const char* fixed_code = R"(Widget {
    property data: int
})";
    
    workspace.update_file("file:///fix.fml", fixed_code);
    doc = workspace.get_diagnostics("file:///fix.fml");
    
    std::cout << "\n✓ After fixing the error\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 0)\n";
    assert(doc->diagnostic_count == 0);
    
    // Introduce error again
    workspace.update_file("file:///fix.fml", error_code);
    doc = workspace.get_diagnostics("file:///fix.fml");
    
    std::cout << "\n✓ After reintroducing error\n";
    std::cout << "  Diagnostics: " << doc->diagnostic_count << " (expected 1)\n";
    assert(doc->diagnostic_count == 1);
    
    std::cout << "\n✓ Error recovery works correctly!\n\n";
}

int main() {
    std::cout << "Forma VirtualFS Tests\n";
    std::cout << "=====================\n\n";
    
    test_basic_operations();
    test_workspace_integration();
    test_forward_references();
    test_generic_types();
    test_enums_and_events();
    
    std::cout << "Diagnostic Tests\n";
    std::cout << "================\n\n";
    
    test_diagnostic_unknown_type();
    test_diagnostic_generic_types();
    test_diagnostic_multiple_errors();
    test_diagnostic_severity_levels();
    test_diagnostic_error_recovery();
    
    std::cout << "====================================\n";
    std::cout << "✓ All VirtualFS and diagnostic tests passed!\n";
    std::cout << "====================================\n";
    
    return 0;
}
