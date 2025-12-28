#include "src/forma.hpp"
#include "src/ir.hpp"
#include "src/lvgl_renderer.hpp"
#include <iostream>

// Parse Forma code and generate LVGL C code
void compile_forma_to_lvgl(const char* forma_code) {
    ::std::cout << "Forma Source Code:\n";
    ::std::cout << "------------------\n";
    ::std::cout << forma_code << "\n\n";
    
    // Create document to hold parsed data
    Document<32, 16, 16, 32, 64> doc;
    Parser parser(forma_code);
    
    // Parse all declarations
    while (!parser.check(TokenKind::EndOfFile)) {
        // Skip whitespace/newlines
        if (parser.current.kind == TokenKind::EndOfFile) break;
        
        if (parser.current.kind == TokenKind::Enum) {
            if (doc.enum_count < 16) {
                doc.enums[doc.enum_count++] = parse_enum(parser);
            }
        } else if (parser.current.kind == TokenKind::Event) {
            if (doc.event_count < 16) {
                doc.events[doc.event_count++] = parse_event(parser);
            }
        } else if (parser.current.kind == TokenKind::Identifier) {
            // Could be type def or instance
            auto saved_pos = parser.lexer.pos;
            auto type_or_name = parser.current.text;
            parser.advance();
            
            if (parser.check(TokenKind::LBrace)) {
                // It's an instance
                parser.lexer.pos = saved_pos;
                parser.current = next_token(parser.lexer);
                auto inst = parse_instance(parser);
                doc.instances.add_instance(inst);
            } else if (parser.check(TokenKind::Colon)) {
                // Type with base: Name: Base { }
                parser.lexer.pos = saved_pos;
                parser.current = next_token(parser.lexer);
                if (doc.type_count < 32) {
                    doc.types[doc.type_count++] = parse_type_decl(parser);
                }
            } else {
                // Unknown - skip
                break;
            }
        } else {
            // Unknown token, advance
            parser.advance();
        }
    }
    
    // Generate LVGL code
    forma::lvgl::LVGLRenderer<16384> renderer;
    renderer.generate(doc);
    
    ::std::cout << "Generated LVGL C99 Code:\n";
    ::std::cout << "------------------------\n";
    ::std::cout << renderer.c_str() << "\n";
}

int main() {
    ::std::cout << "========================================\n";
    ::std::cout << "Forma to LVGL Compiler Demo\n";
    ::std::cout << "========================================\n\n";
    
    // Example 1: Simple button
    ::std::cout << "Example 1: Simple Button\n";
    ::std::cout << "========================\n\n";
    
    const char* example1 = R"(
Button {
    text: "Click Me"
    x: 10
    y: 20
}
)";
    compile_forma_to_lvgl(example1);
    
    ::std::cout << "\n\n";
    
    // Example 2: Dashboard with multiple widgets
    ::std::cout << "Example 2: Dashboard\n";
    ::std::cout << "====================\n\n";
    
    const char* example2 = R"(
enum Status {
    Offline,
    Online,
    Error
}

Panel {
    width: 480
    height: 320
}

Label {
    text: "System Dashboard"
    x: 10
    y: 10
}

Slider {
    value: 50
    x: 10
    y: 50
    width: 300
}

Button {
    text: "Start"
    x: 10
    y: 100
}

Button {
    text: "Stop"
    x: 120
    y: 100
}

Label {
    text: "Status: Ready"
    x: 10
    y: 150
}
)";
    compile_forma_to_lvgl(example2);
    
    ::std::cout << "\n\n";
    
    // Example 3: Type definition with enum
    ::std::cout << "Example 3: Settings Panel\n";
    ::std::cout << "=========================\n\n";
    
    const char* example3 = R"(
enum Theme {
    Light,
    Dark,
    Auto
}

SettingsPanel {
    property title: string
    property brightness: int
    property theme: Theme
}

Panel {
    width: 320
    height: 240
}

Label {
    text: "Settings"
    x: 10
    y: 10
}

Checkbox {
    checked: true
    x: 10
    y: 50
}

Switch {
    x: 10
    y: 100
}
)";
    compile_forma_to_lvgl(example3);
    
    return 0;
}
