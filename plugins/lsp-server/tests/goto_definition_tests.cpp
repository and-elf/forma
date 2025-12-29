#include <bugspray/bugspray.hpp>
#include "../src/lsp.hpp"

TEST_CASE("LSP - Go to Definition")
{
    SECTION("Go to type definition")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = R"(
Button {
    property text: string
}

App {
    button: Button
}
)";
        manager.did_open(item);
        
        // Try to find definition of "Button" at line 6
        forma::lsp::Location location;
        forma::lsp::Position pos(6, 12); // Position at "Button" in "button: Button"
        
        bool found = manager.find_definition("file:///test.fml", pos, location);
        
        CHECK(found == true);
        if (found) {
            CHECK(location.range.start.line == 1);
        }
    }
    
    SECTION("Go to property definition")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = R"(
Button {
    property enabled: bool
    property text: string
}

App {
    myButton: Button {
        enabled: true
    }
}
)";
        manager.did_open(item);
        
        // Try to find definition of "enabled" at line 8
        forma::lsp::Location location;
        forma::lsp::Position pos(8, 8); // Position at "enabled: true"
        
        bool found = manager.find_definition("file:///test.fml", pos, location);
        
        CHECK(found == true);
        if (found) {
            CHECK(location.range.start.line == 2);
        }
    }
    
    SECTION("Definition not found for unknown identifier")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = "Button { property text: UnknownType }";
        manager.did_open(item);
        
        forma::lsp::Location location;
        forma::lsp::Position pos(0, 24); // Position at "UnknownType"
        
        bool found = manager.find_definition("file:///test.fml", pos, location);
        
        CHECK(found == false);
    }
    
    SECTION("Go to definition with multiline source")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = R"(// Comment
type Color = enum {
    Red,
    Green,
    Blue
}

Button {
    property color: Color
}
)";
        manager.did_open(item);
        
        // Try to find definition of "Color" at line 8 (in property declaration)
        forma::lsp::Location location;
        forma::lsp::Position pos(8, 20); // Position at "Color" in "property color: Color"
        
        bool found = manager.find_definition("file:///test.fml", pos, location);
        
        CHECK(found == true);
        if (found) {
            CHECK(location.range.start.line == 1);
        }
    }
}

TEST_CASE("LSP - Hover Information")
{
    SECTION("Hover shows type information")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = R"(
Button {
    property text: string
}
)";
        manager.did_open(item);
        
        // Note: hover functionality is not implemented yet in the LSP
        // This test documents expected behavior
        
        // Hover over "text" should show: "property text: string"
        // Hover over "Button" should show: "type Button"
    }
}
