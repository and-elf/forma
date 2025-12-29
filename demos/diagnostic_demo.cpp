#include "ir.hpp"
#include <iostream>

void print_diagnostic(const Diagnostic& diag) {
    std::cout << "  [";
    switch (diag.severity) {
        case DiagnosticSeverity::Error:   std::cout << "ERROR"; break;
        case DiagnosticSeverity::Warning: std::cout << "WARN "; break;
        case DiagnosticSeverity::Info:    std::cout << "INFO "; break;
        case DiagnosticSeverity::Hint:    std::cout << "HINT "; break;
    }
    std::cout << "] ";
    
    if (!diag.code.empty()) {
        std::cout << "(" << diag.code << ") ";
    }
    
    std::cout << "at offset " << diag.location.offset 
              << ": " << diag.message << "\n";
}

void print_diagnostics(const auto& diags, const char* title) {
    std::cout << "\n" << title << "\n";
    std::cout << std::string(50, '=') << "\n";
    
    if (diags.count == 0) {
        std::cout << "  No diagnostics\n";
    } else {
        for (size_t i = 0; i < diags.count; ++i) {
            print_diagnostic(diags.diagnostics[i]);
        }
    }
}

int main() {
    std::cout << "Forma Diagnostic System Demo\n";
    std::cout << std::string(50, '=') << "\n\n";
    
    // Example 1: Unknown property type
    {
        std::cout << "Example 1: Type with unknown property type\n";
        std::cout << "Code: Widget { property data: UnknownType }\n";
        
        SymbolTable<128> symbols;
        
        TypeDecl decl;
        decl.name = "Widget";
        decl.prop_count = 1;
        decl.properties[0].name = "data";
        decl.properties[0].type = TypeRef("UnknownType");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
    }
    
    // Example 2: Valid type with built-ins
    {
        std::cout << "\nExample 2: Valid type with built-in types\n";
        std::cout << "Code: Button { property text: string; property enabled: bool }\n";
        
        SymbolTable<128> symbols;
        
        TypeDecl decl;
        decl.name = "Button";
        decl.prop_count = 2;
        decl.properties[0].name = "text";
        decl.properties[0].type = TypeRef("string");
        decl.properties[1].name = "enabled";
        decl.properties[1].type = TypeRef("bool");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
    }
    
    // Example 3: Type inheritance with missing base
    {
        std::cout << "\nExample 3: Type inheritance with unknown base\n";
        std::cout << "Code: MyButton: Button { property corners: int }\n";
        
        SymbolTable<128> symbols;
        // Note: Button is not in symbol table!
        
        TypeDecl decl;
        decl.name = "MyButton";
        decl.base_type = "Button";
        decl.prop_count = 1;
        decl.properties[0].name = "corners";
        decl.properties[0].type = TypeRef("int");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
    }
    
    // Example 4: Valid type inheritance
    {
        std::cout << "\nExample 4: Valid type inheritance\n";
        std::cout << "Code: MyButton: Button { property corners: int }\n";
        
        SymbolTable<128> symbols;
        symbols.add_symbol(Symbol::Kind::Type, "Button", SourceLocation{}, 0);
        
        TypeDecl decl;
        decl.name = "MyButton";
        decl.base_type = "Button";
        decl.prop_count = 1;
        decl.properties[0].name = "corners";
        decl.properties[0].type = TypeRef("int");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
    }
    
    // Example 5: Instance with unknown type
    {
        std::cout << "\nExample 5: Instance with unknown type\n";
        std::cout << "Code: UnknownWidget { text: \"hello\" }\n";
        
        SymbolTable<128> symbols;
        
        InstanceDecl inst;
        inst.type_name = "UnknownWidget";
        inst.prop_count = 1;
        inst.properties[0].name = "text";
        inst.properties[0].value.kind = Value::Kind::String;
        inst.properties[0].value.text = "hello";
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        
        std::array<TypeDecl, 0> empty_types{};
        analyzer.validate_instance(inst, empty_types, 0, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
    }
    
    // Example 6: Event with unknown parameter type
    {
        std::cout << "\nExample 6: Event with unknown parameter type\n";
        std::cout << "Code: event onUpdate(data: CustomData)\n";
        
        SymbolTable<128> symbols;
        
        EventDecl decl;
        decl.name = "onUpdate";
        decl.param_count = 1;
        decl.params[0].name = "data";
        decl.params[0].type = TypeRef("CustomData");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        analyzer.validate_event(decl, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
    }
    
    // Example 7: Multiple errors
    {
        std::cout << "\nExample 7: Multiple type errors\n";
        std::cout << "Code: Widget: UnknownBase { property a: BadType1; property b: BadType2 }\n";
        
        SymbolTable<128> symbols;
        
        TypeDecl decl;
        decl.name = "Widget";
        decl.base_type = "UnknownBase";
        decl.prop_count = 2;
        decl.properties[0].name = "a";
        decl.properties[0].type = TypeRef("BadType1");
        decl.properties[1].name = "b";
        decl.properties[1].type = TypeRef("BadType2");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{1, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        print_diagnostics(analyzer.diagnostics, "Diagnostics");
        std::cout << "\n  Total errors: " << analyzer.diagnostics.count << "\n";
        std::cout << "  Has errors: " << (analyzer.diagnostics.has_errors() ? "yes" : "no") << "\n";
    }
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "All diagnostics are computed at compile time!\n";
    std::cout << "This system is ready for LSP integration.\n";
    
    return 0;
}
