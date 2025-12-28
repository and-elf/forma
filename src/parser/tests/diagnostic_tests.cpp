#include <bugspray/bugspray.hpp>
#include "ir.hpp"

using namespace forma;

TEST_CASE("Diagnostics - Type Validation")
{
    SECTION("Unknown property type")
    {
        SymbolTable<128> symbols;
        
        TypeDecl decl;
        decl.name = "Widget";
        decl.prop_count = 1;
        decl.properties[0].name = "value";
        decl.properties[0].type = TypeRef("UnknownType");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        CHECK(analyzer.diagnostics.count == 1);
        CHECK(analyzer.diagnostics.diagnostics[0].severity == DiagnosticSeverity::Error);
        CHECK(analyzer.diagnostics.diagnostics[0].code == "unknown-type");
    }
    
    SECTION("Built-in types are valid")
    {
        SymbolTable<128> symbols;
        
        TypeDecl decl;
        decl.name = "Widget";
        decl.prop_count = 4;
        decl.properties[0].name = "intValue";
        decl.properties[0].type = TypeRef("int");
        decl.properties[1].name = "floatValue";
        decl.properties[1].type = TypeRef("float");
        decl.properties[2].name = "stringValue";
        decl.properties[2].type = TypeRef("string");
        decl.properties[3].name = "boolValue";
        decl.properties[3].type = TypeRef("bool");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        CHECK(analyzer.diagnostics.count == 0ul);
    }
    
    SECTION("Unknown base type")
    {
        SymbolTable<128> symbols;
        
        TypeDecl decl;
        decl.name = "MyWidget";
        decl.base_type = "UnknownBase";
        decl.prop_count = 0;
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        CHECK(analyzer.diagnostics.count == 1ul);
        CHECK(analyzer.diagnostics.diagnostics[0].severity == DiagnosticSeverity::Error);
        CHECK(analyzer.diagnostics.diagnostics[0].code == "unknown-type");
    }
    
    SECTION("Valid base type from symbol table")
    {
        SymbolTable<128> symbols;
        symbols.add_symbol(Symbol::Kind::Type, "Rectangle", SourceLocation{}, 0);
        
        TypeDecl decl;
        decl.name = "MyRect";
        decl.base_type = "Rectangle";
        decl.prop_count = 1;
        decl.properties[0].name = "cornerRadius";
        decl.properties[0].type = TypeRef("int");
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_type_decl(decl, loc);
        
        CHECK(analyzer.diagnostics.count == 0ul);
    }
}

TEST_CASE("Diagnostics - Instance Validation")
{
    SECTION("Unknown instance type")
    {
        SymbolTable<128> symbols;
        
        InstanceDecl inst;
        inst.type_name = "UnknownWidget";
        inst.prop_count = 0;
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_instance(inst, loc);
        
        CHECK(analyzer.diagnostics.count == 1ul);
        CHECK(analyzer.diagnostics.diagnostics[0].severity == DiagnosticSeverity::Error);
    }
    
    SECTION("Valid instance type from symbol table")
    {
        SymbolTable<128> symbols;
        symbols.add_symbol(Symbol::Kind::Type, "Rectangle", SourceLocation{}, 0);
        
        InstanceDecl inst;
        inst.type_name = "Rectangle";
        inst.prop_count = 0;
        
        SemanticAnalyzer<64> analyzer(&symbols);
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_instance(inst, loc);
        
        CHECK(analyzer.diagnostics.count == 0ul);
    }
}

TEST_CASE("Diagnostics - Error Reporting")
{
    SECTION("Diagnostic formatting")
    {
        Diagnostic diag;
        diag.severity = DiagnosticSeverity::Error;
        diag.message = "Test error message";
        diag.code = "test-error";
        diag.location = SourceLocation{1, 10, 1, 20};
        
        CHECK(diag.severity == DiagnosticSeverity::Error);
        CHECK(diag.message == "Test error message");
        CHECK(diag.code == "test-error");
    }
    
    SECTION("Multiple diagnostics")
    {
        DiagnosticList<16> diagnostics;
        
        diagnostics.add(DiagnosticSeverity::Warning, "First warning", SourceLocation{}, "warn-1");
        diagnostics.add(DiagnosticSeverity::Error, "First error", SourceLocation{}, "err-1");
        diagnostics.add(DiagnosticSeverity::Warning, "Second warning", SourceLocation{}, "warn-2");
        
        CHECK(diagnostics.count == 3ul);
        CHECK(diagnostics.diagnostics[0].severity == DiagnosticSeverity::Warning);
        CHECK(diagnostics.diagnostics[1].severity == DiagnosticSeverity::Error);
        CHECK(diagnostics.diagnostics[2].severity == DiagnosticSeverity::Warning);
    }
}
