#pragma once

#include "lsp.hpp"

namespace forma::lsp::tests {

// ============================================================================
// LSP Protocol Type Tests
// ============================================================================

constexpr bool test_position() {
    Position pos(10, 5);
    return pos.line == 10 && pos.character == 5;
}

constexpr bool test_range() {
    Range range(1, 2, 3, 4);
    return range.start.line == 1 
        && range.start.character == 2
        && range.end.line == 3 
        && range.end.character == 4;
}

constexpr bool test_location() {
    Location loc("file:///test.fml", Range(0, 0, 0, 10));
    return loc.uri == "file:///test.fml"
        && loc.range.start.line == 0
        && loc.range.end.character == 10;
}

constexpr bool test_diagnostic() {
    Diagnostic diag(Range(1, 0, 1, 10), DiagnosticSeverity::Error, 
                    "unknown-type", "Unknown type 'Foo'");
    return diag.range.start.line == 1
        && diag.severity == DiagnosticSeverity::Error
        && diag.code == "unknown-type"
        && diag.message == "Unknown type 'Foo'";
}

// ============================================================================
// LSP Server Tests
// ============================================================================

constexpr bool test_lsp_initialize() {
    LSPDocumentManager<> manager;
    
    auto result = manager.initialize(1234, "file:///workspace");
    
    return manager.initialized == true
        && result.server_name == "forma-lsp"
        && result.server_version == "0.1.0";
}

constexpr bool test_lsp_did_open() {
    LSPDocumentManager<> manager;
    manager.initialized = true;
    
    TextDocumentItem item;
    item.uri = "file:///test.fml";
    item.language_id = "forma";
    item.version = 1;
    item.text = "Button { property text: string }";
    
    manager.did_open(item);
    
    auto doc = manager.find_document("file:///test.fml");
    
    return manager.document_count == 1
        && doc != nullptr
        && doc->uri == "file:///test.fml"
        && doc->text == "Button { property text: string }"
        && doc->version == 1
        && doc->active == true;
}

constexpr bool test_lsp_did_change() {
    LSPDocumentManager<> manager;
    manager.initialized = true;
    
    // First open a document
    TextDocumentItem item;
    item.uri = "file:///test.fml";
    item.language_id = "forma";
    item.version = 1;
    item.text = "Button { }";
    
    manager.did_open(item);
    
    // Now change it
    VersionedTextDocumentIdentifier id;
    id.uri = "file:///test.fml";
    id.version = 2;
    
    manager.did_change(id, "Button { property enabled: bool }");
    
    auto doc = manager.find_document("file:///test.fml");
    
    return doc != nullptr
        && doc->text == "Button { property enabled: bool }"
        && doc->version == 2;
}

constexpr bool test_lsp_did_close() {
    LSPDocumentManager<> manager;
    manager.initialized = true;
    
    // First open a document
    TextDocumentItem item;
    item.uri = "file:///test.fml";
    item.language_id = "forma";
    item.version = 1;
    item.text = "Button { }";
    
    manager.did_open(item);
    
    // Now close it
    TextDocumentIdentifier id("file:///test.fml");
    manager.did_close(id);
    
    auto doc = manager.find_document("file:///test.fml");
    
    return doc == nullptr;  // Should not find closed documents
}

constexpr bool test_lsp_diagnostics_valid_code() {
    LSPDocumentManager<> manager;
    manager.initialized = true;
    
    TextDocumentItem item;
    item.uri = "file:///test.fml";
    item.language_id = "forma";
    item.version = 1;
    item.text = "Button { property text: string }";
    
    manager.did_open(item);
    
    auto doc = manager.find_document("file:///test.fml");
    
    return doc != nullptr
        && doc->diagnostic_count == 0;  // Valid code = no diagnostics
}

constexpr bool test_lsp_diagnostics_unknown_type() {
    LSPDocumentManager<> manager;
    manager.initialized = true;
    
    TextDocumentItem item;
    item.uri = "file:///test.fml";
    item.language_id = "forma";
    item.version = 1;
    item.text = "Widget { property data: UnknownType }";
    
    manager.did_open(item);
    
    auto doc = manager.find_document("file:///test.fml");
    
    return doc != nullptr
        && doc->diagnostic_count > 0
        && doc->diagnostics[0].severity == DiagnosticSeverity::Error
        && doc->diagnostics[0].code == "unknown-type";
}

// ============================================================================
// Test Runner
// ============================================================================

constexpr bool run_all_lsp_tests() {
    return test_position()
        && test_range()
        && test_location()
        && test_diagnostic()
        && test_lsp_initialize()
        && test_lsp_did_open()
        && test_lsp_did_change()
        && test_lsp_did_close()
        && test_lsp_diagnostics_valid_code()
        && test_lsp_diagnostics_unknown_type();
}

// Compile-time assertions
static_assert(test_position(), "test_position failed");
static_assert(test_range(), "test_range failed");
static_assert(test_location(), "test_location failed");
static_assert(test_diagnostic(), "test_diagnostic failed");
static_assert(test_lsp_initialize(), "test_lsp_initialize failed");
static_assert(test_lsp_did_open(), "test_lsp_did_open failed");
static_assert(test_lsp_did_change(), "test_lsp_did_change failed");
static_assert(test_lsp_did_close(), "test_lsp_did_close failed");
static_assert(test_lsp_diagnostics_valid_code(), "test_lsp_diagnostics_valid_code failed");
static_assert(test_lsp_diagnostics_unknown_type(), "test_lsp_diagnostics_unknown_type failed");

static_assert(run_all_lsp_tests(), "LSP tests failed at compile time!");

} // namespace forma::lsp::tests
