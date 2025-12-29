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
class CustomWidget {
    property text: string
}

class App {
    property myWidget: CustomWidget
}
)";
        manager.did_open(item);
        
        // Try to find definition of "CustomWidget" at line 6 (in the type annotation)
        forma::lsp::Location location;
        forma::lsp::Position pos(6, 23); // Position at "CustomWidget" in "property myWidget: CustomWidget"
        
        bool found = manager.find_definition("file:///test.fml", pos, location);
        
        CHECK(found == true);
        if (found) {
            CHECK(location.range.start.line == 1); // Points to "class CustomWidget"
        }
    }
    
    SECTION("Go to property definition")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = R"(
class MyButton {
    property enabled: bool
    property text: string
}

myBtn: MyButton {
    enabled: true
}
)";
        manager.did_open(item);
        
        // Try to find definition of "enabled" property
        forma::lsp::Location location;
        forma::lsp::Position pos(7, 4); // Position at "enabled: true"
        
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
        item.text = "class Widget { property data: UnknownType }";
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
enum Color {
    Red,
    Green,
    Blue
}

class CustomButton {
    property color: Color
}
)";
        manager.did_open(item);
        
        // Try to find definition of "Color" enum at line 8 (in property declaration)
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
class MyWidget {
    property text: string
}
)";
        manager.did_open(item);
        
        // Note: hover functionality is not implemented yet in the LSP
        // This test documents expected behavior
        
        // Hover over "text" should show: "property text: string"
        // Hover over "MyWidget" should show: "type MyWidget"
    }
}
