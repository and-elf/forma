#include <bugspray/bugspray.hpp>
#include "../src/lsp.hpp"

TEST_CASE("LSP - LVGL Type Resolution")
{
    SECTION("Button type resolution")
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
    
    SECTION("Label type resolution")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = "Label { property text: string property width: int }";
        
        manager.did_open(item);
        auto doc = manager.find_document("file:///test.fml");
        
        REQUIRE(doc != nullptr);
        CHECK(doc->diagnostic_count == static_cast<size_t>(0));
    }
    
    SECTION("Slider type resolution")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = "Slider { property value: int property min: int property max: int }";
        
        manager.did_open(item);
        auto doc = manager.find_document("file:///test.fml");
        
        REQUIRE(doc != nullptr);
        CHECK(doc->diagnostic_count == static_cast<size_t>(0));
    }
    
    SECTION("All LVGL widget types resolve correctly")
    {
        const char* widget_types[] = {
            "Switch", "Checkbox", "Panel", "Container", "Dropdown", 
            "TextArea", "Image", "Arc", "Bar", "Spinner", "List", 
            "Chart", "Table", "Calendar", "Keyboard", "Roller"
        };
        
        for (const char* widget : widget_types) {
            forma::lsp::LSPDocumentManager<> manager;
            manager.initialized = true;
            
            forma::lsp::TextDocumentItem item;
            item.uri = "file:///test.fml";
            std::string text = std::string(widget) + " { property data: int }";
            item.text = text;
            
            manager.did_open(item);
            auto doc = manager.find_document("file:///test.fml");
            
            REQUIRE(doc != nullptr);
            CHECK(doc->diagnostic_count == static_cast<size_t>(0));
        }
    }
    
    SECTION("Unknown type produces diagnostic error")
    {
        forma::lsp::LSPDocumentManager<> manager;
        manager.initialized = true;
        
        forma::lsp::TextDocumentItem item;
        item.uri = "file:///test.fml";
        item.text = "UnknownWidget { property data: string }";
        
        manager.did_open(item);
        auto doc = manager.find_document("file:///test.fml");
        
        REQUIRE(doc != nullptr);
        CHECK(doc->diagnostic_count > static_cast<size_t>(0));
    }
}
