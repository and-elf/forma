#pragma once
#include "diagnostics.hpp"
#include "ir_types.hpp"
#include "tokenizer.hpp"

namespace forma {

// ============================================================================
// Semantic Analysis & Type Resolution
// ============================================================================

template <size_t MaxDiags = 64>
struct SemanticAnalyzer {
    DiagnosticList<MaxDiags> diagnostics;
    const SymbolTable<128>* symbols = nullptr;
    
    constexpr SemanticAnalyzer(const SymbolTable<128>* sym_table) 
        : symbols(sym_table) {}
    
    // Helper to create source location from token
    constexpr SourceLocation loc_from_token(const Tok& tok) const {
        return SourceLocation{0, 0, tok.pos, tok.text.size()};
    }
    
    // Validate that a type name exists in the symbol table
    constexpr bool validate_type(std::string_view type_name, const Tok& tok) {
        if (type_name.empty()) {
            return true; // Empty type means optional/not set
        }
        
        // Check built-in types
        if (type_name == "int" || type_name == "float" || type_name == "string" || 
            type_name == "bool" || type_name == "void") {
            return true;
        }
        
        // Check LVGL widget types (built-in widgets)
        if (type_name == "Button" || type_name == "Label" || type_name == "Panel" || 
            type_name == "Container" || type_name == "Slider" || type_name == "Switch" ||
            type_name == "Checkbox" || type_name == "Dropdown" || type_name == "TextArea" ||
            type_name == "Image" || type_name == "Arc" || type_name == "Bar" ||
            type_name == "Spinner" || type_name == "List" || type_name == "Chart" ||
            type_name == "Table" || type_name == "Calendar" || type_name == "Keyboard" ||
            type_name == "Roller" || type_name == "Textarea") {
            return true;
        }
        
        // Check if type exists in symbol table
        if (!symbols->exists(type_name)) {
            auto loc = loc_from_token(tok);
            diagnostics.add(DiagnosticSeverity::Error, 
                          type_name, 
                          loc, 
                          "unknown-type");
            return false;
        }
        
        return true;
    }
    
    // Check if a value kind matches a type
    constexpr bool value_matches_type(const Value& value, const TypeRef& type_ref) {
        // Check built-in type compatibility
        if (type_ref.name == "int") {
            return value.kind == Value::Kind::Integer;
        }
        if (type_ref.name == "float") {
            return value.kind == Value::Kind::Integer || value.kind == Value::Kind::Float;
        }
        if (type_ref.name == "string") {
            return value.kind == Value::Kind::String;
        }
        if (type_ref.name == "bool") {
            return value.kind == Value::Kind::Bool;
        }
        // For custom types, identifiers should be valid
        return value.kind == Value::Kind::Identifier;
    }
    
    // Validate a TypeRef (can be simple or generic)
    constexpr bool validate_type_ref(const TypeRef& type_ref, SourceLocation loc) {
        if (type_ref.name.empty()) {
            return true;
        }
        
        // Check built-in generic types
        if (type_ref.name == "Forma.Array") {
            // Forma.Array requires exactly 2 parameters: (Type, Size)
            if (type_ref.param_count != 2) {
                diagnostics.add(DiagnosticSeverity::Error,
                              "Forma.Array requires 2 parameters (Type, Size)",
                              loc,
                              "invalid-generic-params");
                return false;
            }
            
            // First parameter must be a type
            if (type_ref.params[0].kind != TypeParam::Kind::Type) {
                diagnostics.add(DiagnosticSeverity::Error,
                              "First parameter must be a type",
                              loc,
                              "invalid-generic-params");
                return false;
            }
            
            // Second parameter must be an integer
            if (type_ref.params[1].kind != TypeParam::Kind::Integer) {
                diagnostics.add(DiagnosticSeverity::Error,
                              "Second parameter must be an integer",
                              loc,
                              "invalid-generic-params");
                return false;
            }
            
            // Validate the element type
            Tok tok{TokenKind::Identifier, type_ref.params[0].value, loc.offset};
            return validate_type(type_ref.params[0].value, tok);
        }
        
        // For simple types, validate the name
        if (!type_ref.is_generic()) {
            Tok tok{TokenKind::Identifier, type_ref.name, loc.offset};
            return validate_type(type_ref.name, tok);
        }
        
        // Unknown generic type
        diagnostics.add(DiagnosticSeverity::Error,
                      type_ref.name,
                      loc,
                      "unknown-generic-type");
        return false;
    }
    
    // Validate TypeDecl
    constexpr void validate_type_decl(const TypeDecl& decl, SourceLocation loc) {
        // Check for base type if it exists
        if (!decl.base_type.empty()) {
            // Create a fake token for location
            Tok tok{TokenKind::Identifier, decl.base_type, loc.offset};
            validate_type(decl.base_type, tok);
        }
        
        // Validate property types
        for (size_t i = 0; i < decl.prop_count; ++i) {
            const auto& prop = decl.properties[i];
            validate_type_ref(prop.type, loc);
        }
        
        // Validate method return types and parameters
        for (size_t i = 0; i < decl.method_count; ++i) {
            const auto& method = decl.methods[i];
            validate_type_ref(method.return_type, loc);
            
            for (size_t j = 0; j < method.param_count; ++j) {
                validate_type_ref(method.params[j].type, loc);
            }
        }
    }
    
    // Find a type declaration by name
    template <size_t MaxTypes>
    constexpr const TypeDecl* find_type_decl(std::string_view name,
                                             const std::array<TypeDecl, MaxTypes>& types,
                                             size_t type_count) const {
        for (size_t i = 0; i < type_count; ++i) {
            if (types[i].name == name) {
                return &types[i];
            }
        }
        return nullptr;
    }
    
    // Validate InstanceDecl
    template <size_t MaxTypes>
    constexpr void validate_instance(const InstanceDecl& inst, 
                                    const std::array<TypeDecl, MaxTypes>& types,
                                    size_t type_count,
                                    SourceLocation loc) {
        // Validate instance type exists
        Tok tok{TokenKind::Identifier, inst.type_name, loc.offset};
        if (!validate_type(inst.type_name, tok)) {
            return; // Don't validate properties if type doesn't exist
        }
        
        // Find the type declaration
        const TypeDecl* type_decl = find_type_decl(inst.type_name, types, type_count);
        if (!type_decl) {
            // Type might be built-in or LVGL widget - skip property validation
            return;
        }
        
        // Validate each property assignment
        for (size_t i = 0; i < inst.prop_count; ++i) {
            const auto& prop_assignment = inst.properties[i];
            
            // Find the property in the type declaration
            const PropertyDecl* prop_decl = nullptr;
            for (size_t j = 0; j < type_decl->prop_count; ++j) {
                if (type_decl->properties[j].name == prop_assignment.name) {
                    prop_decl = &type_decl->properties[j];
                    break;
                }
            }
            
            if (!prop_decl) {
                // Property not found in type declaration  
                diagnostics.add(DiagnosticSeverity::Error,
                              prop_assignment.name,
                              loc,
                              "unknown-property");
                continue;
            }
            
            // Validate property value type matches declaration
            if (!value_matches_type(prop_assignment.value, prop_decl->type)) {
                diagnostics.add(DiagnosticSeverity::Error,
                              prop_assignment.name,
                              loc,
                              "type-mismatch");
            }
            
            // Validate preview value if present
            if (prop_assignment.has_preview) {
                if (!value_matches_type(prop_assignment.preview_value, prop_decl->type)) {
                    diagnostics.add(DiagnosticSeverity::Error,
                                  prop_assignment.name,
                                  loc,
                                  "type-mismatch-preview");
                }
            }
        }
    }
    
    // Validate EventDecl
    constexpr void validate_event(const EventDecl& decl, SourceLocation loc) {
        for (size_t i = 0; i < decl.param_count; ++i) {
            const auto& param = decl.params[i];
            validate_type_ref(param.type, loc);
        }
    }
    
    // Check for duplicate declarations
    constexpr void check_duplicate(std::string_view name, SourceLocation loc, 
                                   std::string_view kind) {
        const Symbol* existing = symbols->find(name);
        if (existing) {
            diagnostics.add(DiagnosticSeverity::Error,
                          name,
                          loc,
                          "duplicate-declaration");
        }
    }
};

// Analyze a complete document
template <size_t MaxDiags = 64, size_t MaxTypes = 32, size_t MaxEnums = 16, 
          size_t MaxEvents = 16, size_t MaxImports = 32, size_t MaxInstances = 64, size_t MaxAssets = 64>
constexpr DiagnosticList<MaxDiags> analyze_document(
    Document<MaxTypes, MaxEnums, MaxEvents, MaxImports, MaxInstances, MaxAssets>& doc) {
    
    SemanticAnalyzer<MaxDiags> analyzer(&doc.symbols);
    
    // Validate all type declarations
    for (size_t i = 0; i < doc.type_count; ++i) {
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_type_decl(doc.types[i], loc);
    }
    
    // Validate all event declarations
    for (size_t i = 0; i < doc.event_count; ++i) {
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_event(doc.events[i], loc);
    }
    
    // Validate all instances with property type checking
    for (size_t i = 0; i < doc.instances.count; ++i) {
        SourceLocation loc{0, 0, 0, 0};
        analyzer.validate_instance(doc.instances.get(i), doc.types, doc.type_count, loc);
    }
    
    return analyzer.diagnostics;
}

} // namespace forma
