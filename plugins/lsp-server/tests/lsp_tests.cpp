#include <bugspray/bugspray.hpp>
#include "../src/lsp.hpp"

TEST_CASE("LSP - Protocol Types")
{
    SECTION("Position")
    {
        forma::lsp::Position pos(10, 5);
        CHECK(pos.line == 10);
        CHECK(pos.character == 5);
    }
    
    SECTION("Range")
    {
        forma::lsp::Range range(1, 2, 3, 4);
        CHECK(range.start.line == 1);
        CHECK(range.start.character == 2);
        CHECK(range.end.line == 3);
        CHECK(range.end.character == 4);
    }
    
    SECTION("Location")
    {
        forma::lsp::Location loc("file:///test.fml", forma::lsp::Range(0, 0, 0, 10));
        CHECK(loc.uri == "file:///test.fml");
        CHECK(loc.range.start.line == 0);
        CHECK(loc.range.end.character == 10);
    }
    
    SECTION("Diagnostic")
    {
        forma::lsp::Diagnostic diag(forma::lsp::Range(1, 0, 1, 10), 
                                     forma::lsp::DiagnosticSeverity::Error, 
                                     "unknown-type", "Unknown type 'Foo'");
        CHECK(diag.range.start.line == 1);
        CHECK(diag.severity == forma::lsp::DiagnosticSeverity::Error);
        CHECK(diag.code == "unknown-type");
        CHECK(diag.message == "Unknown type 'Foo'");
    }
}

TEST_CASE("LSP - Document Manager")
{
    SECTION("Initialize")
    {
        forma::lsp::LSPDocumentManager<> manager;
        auto result = manager.initialize(1234, "file:///workspace");
        
        CHECK(manager.initialized == true);
        CHECK(result.server_name == "forma-lsp");
        CHECK(result.server_version == "0.1.0");
    }
    
    SECTION("Document open")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.language_id = "forma";
        item.version = 1;
        item.text = "Button { property text: string }";
        
        manager.did_open(item);
        auto doc = manager.find_document("file:///test.fml");
        
        REQUIRE(doc != nullptr);
        CHECK(doc->uri == "file:///test.fml");
        CHECK(doc->text == "Button { property text: string }");
        CHECK(doc->version == 1);
        CHECK(doc->active == true);
    }
    
    SECTION("Document change")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.version = 1;
        item.text = "Button { }";
        manager.did_open(item);
        
        forma::lsp::VersionedTextDocumentIdentifier id;
        id.uri = "file:///test.fml";
        id.version = 2;
        manager.did_change(id, "Button { property enabled: bool }");
        
        auto doc = manager.find_document("file:///test.fml");
        REQUIRE(doc != nullptr);
        CHECK(doc->text == "Button { property enabled: bool }");
        CHECK(doc->version == 2);
    }
    
    SECTION("Document close")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.version = 1;
        item.text = "Button { }";
        manager.did_open(item);
        
        forma::lsp::TextDocumentIdentifier id("file:///test.fml");
        manager.did_close(id);
        
        auto doc = manager.find_document("file:///test.fml");
        CHECK(doc == nullptr);
    }
}

TEST_CASE("LSP - Diagnostics")
{
    SECTION("Valid code produces no diagnostics")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = "Button { property text: string }";
        manager.did_open(item);
        
        auto doc = manager.find_document("file:///test.fml");
        REQUIRE(doc != nullptr);
        CHECK(doc->diagnostic_count == static_cast<size_t>(0));
    }
    
    SECTION("Unknown type produces diagnostic")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = "Widget { property data: UnknownType }";
        manager.did_open(item);
        
        auto doc = manager.find_document("file:///test.fml");
        REQUIRE(doc != nullptr);
        CHECK(doc->diagnostic_count > static_cast<size_t>(0));
        CHECK(doc->diagnostics[0].severity == forma::lsp::DiagnosticSeverity::Error);
        CHECK(doc->diagnostics[0].code == "unknown-type");
    }
}
