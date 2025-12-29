#pragma once
#include "tokenizer.hpp"
#include "ir_types.hpp"

namespace forma {

// ============================================================================
// Forward Declarations
// ============================================================================
struct Parser;
constexpr TypeRef parse_type_ref(Parser& p);
constexpr PropertyDecl parse_property(Parser& p);
constexpr MethodDecl parse_method(Parser& p);
constexpr TypeDecl parse_type_decl(Parser& p);
constexpr InstanceDecl parse_instance(Parser& p);
constexpr EnumDecl parse_enum(Parser& p);
constexpr EventDecl parse_event(Parser& p);
constexpr WhenStmt parse_when(Parser& p);
constexpr AnimationDecl parse_animate(Parser& p);
constexpr ImportDecl parse_import(Parser& p);
constexpr ImportDecl parse_import(Parser& p);

// ============================================================================
// Parser
// ============================================================================

struct Parser {
    Lexer lexer;
    Tok current;
    InstanceNode* node_storage = nullptr; // For storing nested instances
    
    constexpr Parser(std::string_view source) : lexer{source, 0} {
        advance();
    }
    
    constexpr Parser(std::string_view source, InstanceNode* storage) 
        : lexer{source, 0}, node_storage(storage) {
        advance();
    }
    
    constexpr void advance() {
        current = next_token(lexer);
    }
    
    constexpr bool check(TokenKind kind) const {
        return current.kind == kind;
    }
    
    constexpr bool accept(TokenKind kind) {
        if (check(kind)) {
            advance();
            return true;
        }
        return false;
    }
    
    constexpr Tok expect(TokenKind kind) {
        if (current.kind == kind) {
            Tok tok = current;
            advance();
            return tok;
        }
        // In a real implementation, we'd throw or return an error
        return current;
    }
};

// ============================================================================
// Parse Functions
// ============================================================================

// Parse a type reference: simple like 'int' or generic like 'Forma.Array(int, 10)'
constexpr TypeRef parse_type_ref(Parser& p) {
    TypeRef type;
    
    // Parse the base type name (could be dotted like Forma.Array)
    type.name = p.expect(TokenKind::Identifier).text;
    
    // Check for dot notation (Forma.Array)
    if (p.accept(TokenKind::Dot)) {
        auto start = static_cast<size_t>(type.name.data() - p.lexer.src.data());
        Tok next = p.expect(TokenKind::Identifier);
        auto end = static_cast<size_t>(next.text.data() - p.lexer.src.data()) + next.text.size();
        type.name = p.lexer.src.substr(start, end - start);
    }
    
    // Check for generic parameters: (T, N)
    if (p.accept(TokenKind::LParen)) {
        while (!p.check(TokenKind::RParen) && !p.check(TokenKind::EndOfFile)) {
            if (type.param_count < type.params.size()) {
                TypeParam param;
                
                if (p.check(TokenKind::IntegerLiteral)) {
                    param.kind = TypeParam::Kind::Integer;
                    param.value = p.current.text;
                    p.advance();
                } else if (p.check(TokenKind::Identifier)) {
                    param.kind = TypeParam::Kind::Type;
                    param.value = p.current.text;
                    p.advance();
                } else {
                    break;
                }
                
                type.params[type.param_count++] = param;
                p.accept(TokenKind::Comma); // Optional comma
            } else {
                break;
            }
        }
        
        p.expect(TokenKind::RParen);
    }
    
    return type;
}

// Parse: property name: type
constexpr PropertyDecl parse_property(Parser& p) {
    PropertyDecl decl;
    
    p.expect(TokenKind::Property);
    decl.name = p.expect(TokenKind::Identifier).text;
    p.expect(TokenKind::Colon);
    decl.type = parse_type_ref(p);
    
    return decl;
}

// Helper to create TypeRef from just a name
constexpr TypeRef parse_type_ref_from_name(std::string_view name) {
    return TypeRef(name);
}

// Parse: method ReturnType name(param1: type1, param2: type2)
constexpr MethodDecl parse_method(Parser& p) {
    MethodDecl decl;
    
    p.expect(TokenKind::Method);
    
    // Parse return type (optional - defaults to void)
    if (p.check(TokenKind::Identifier)) {
        // Check if next token is also an identifier (means this is return type)
        size_t saved_pos = p.lexer.pos;
        Tok saved_tok = p.current;
        Tok maybe_type = p.current;
        p.advance();
        
        if (p.check(TokenKind::Identifier)) {
            // Previous token was return type
            decl.return_type = parse_type_ref_from_name(maybe_type.text);
            decl.name = p.current.text;
            p.advance();
        } else {
            // No return type, go back
            p.lexer.pos = saved_pos;
            p.current = saved_tok;
            decl.return_type = TypeRef("void");
            decl.name = p.expect(TokenKind::Identifier).text;
        }
    }
    
    // Parse parameters
    p.expect(TokenKind::LParen);
    
    while (!p.check(TokenKind::RParen) && !p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::Identifier)) {
            if (decl.param_count < decl.params.size()) {
                MethodParam param;
                param.name = p.current.text;
                p.advance();
                p.expect(TokenKind::Colon);
                param.type = parse_type_ref(p);
                decl.params[decl.param_count++] = param;
                
                p.accept(TokenKind::Comma);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    p.expect(TokenKind::RParen);
    return decl;
}

// Parse: class TypeName { property declarations } or class TypeName: BaseType { property declarations }
constexpr TypeDecl parse_type_decl(Parser& p) {
    TypeDecl decl;
    
    // Check for @requires annotation
    if (p.accept(TokenKind::At)) {
        p.expect(TokenKind::Requires);
        p.expect(TokenKind::LParen);
        
        // Parse comma-separated list of capabilities
        while (!p.check(TokenKind::RParen) && !p.check(TokenKind::EndOfFile)) {
            if (decl.required_capabilities_count < decl.required_capabilities.size()) {
                decl.required_capabilities[decl.required_capabilities_count++] = 
                    p.expect(TokenKind::Identifier).text;
            }
            
            if (!p.accept(TokenKind::Comma)) {
                break;
            }
        }
        
        p.expect(TokenKind::RParen);
    }
    
    // Optional 'class' keyword
    p.accept(TokenKind::Class);
    
    decl.name = p.expect(TokenKind::Identifier).text;
    
    // Check for inheritance: TypeName: BaseType
    if (p.accept(TokenKind::Colon)) {
        decl.base_type = p.expect(TokenKind::Identifier).text;
    }
    
    p.expect(TokenKind::LBrace);
    
    while (!p.check(TokenKind::RBrace) && !p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::Property)) {
            if (decl.prop_count < decl.properties.size()) {
                decl.properties[decl.prop_count++] = parse_property(p);
            } else {
                break; // Too many properties
            }
        } else if (p.check(TokenKind::Method)) {
            if (decl.method_count < decl.methods.size()) {
                decl.methods[decl.method_count++] = parse_method(p);
            } else {
                break; // Too many methods
            }
        } else {
            break; // Unexpected token
        }
    }
    
    p.expect(TokenKind::RBrace);
    return decl;
}

// Parse a value (literal or identifier)
constexpr Value parse_value(Parser& p) {
    Value val;
    
    if (p.check(TokenKind::IntegerLiteral)) {
        val.kind = Value::Kind::Integer;
        val.text = p.current.text;
        p.advance();
    } else if (p.check(TokenKind::FloatLiteral)) {
        val.kind = Value::Kind::Float;
        val.text = p.current.text;
        p.advance();
    } else if (p.check(TokenKind::StringLiteral)) {
        val.kind = Value::Kind::String;
        val.text = p.current.text;
        p.advance();
    } else if (p.check(TokenKind::BoolLiteral)) {
        val.kind = Value::Kind::Bool;
        val.text = p.current.text;
        p.advance();
    } else if (p.check(TokenKind::Identifier)) {
        val.kind = Value::Kind::Identifier;
        val.text = p.current.text;
        p.advance();
    }
    
    return val;
}

// Parse: name: value or name: value or preview{value}
constexpr PropertyAssignment parse_property_assignment(Parser& p) {
    PropertyAssignment assign;
    
    assign.name = p.expect(TokenKind::Identifier).text;
    p.expect(TokenKind::Colon);
    assign.value = parse_value(p);
    
    // Check for 'or preview{value}'
    if (p.accept(TokenKind::Or)) {
        p.expect(TokenKind::Preview);
        p.expect(TokenKind::LBrace);
        assign.preview_value = parse_value(p);
        assign.has_preview = true;
        p.expect(TokenKind::RBrace);
    }
    
    return assign;
}

// Forward declaration for recursion
constexpr InstanceDecl parse_instance(Parser& p);

// Parse: TypeName { property assignments and child instances }
constexpr InstanceDecl parse_instance(Parser& p) {
    InstanceDecl inst;
    
    inst.type_name = p.expect(TokenKind::Identifier).text;
    p.expect(TokenKind::LBrace);

    while (!p.check(TokenKind::RBrace) && !p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::When)) {
            // Parse when statement
            if (inst.when_count < inst.when_stmts.size()) {
                inst.when_stmts[inst.when_count++] = parse_when(p);
            } else {
                break;  // Too many when statements
            }
        } else if (p.check(TokenKind::Animate)) {
            // Parse animate block
            if (inst.animation_count < inst.animations.size()) {
                inst.animations[inst.animation_count++] = parse_animate(p);
            } else {
                break;  // Too many animations
            }
        } else if (p.check(TokenKind::Identifier)) {
            // Save position to check what comes next
            size_t saved_pos = p.lexer.pos;
            Tok saved_tok = p.current;
            
            p.advance(); // Move past identifier
            
            if (p.check(TokenKind::Colon)) {
                // It's a property assignment
                p.lexer.pos = saved_pos;
                p.current = saved_tok;
                
                if (inst.prop_count < inst.properties.size()) {
                    inst.properties[inst.prop_count++] = parse_property_assignment(p);
                }
            } else if (p.check(TokenKind::LBrace)) {
                // It's a nested instance
                p.lexer.pos = saved_pos;
                p.current = saved_tok;
                
                // Parse child and store in flat array
                if (p.node_storage && p.node_storage->count < InstanceNode::MAX_INSTANCES) {
                    // Parse the child (this recursively handles grandchildren)
                    InstanceDecl child = parse_instance(p);
                    
                    // Add child to flat array and record its index
                    size_t child_idx = p.node_storage->add_instance(child);
                    
                    // Store child index in parent's child_indices array
                    if (inst.child_count < inst.child_indices.size()) {
                        inst.child_indices[inst.child_count] = child_idx;
                        inst.child_count++;
                    }
                }
            } else {
                // Unknown, skip token
                break;
            }
        } else {
            break; // Unexpected token
        }
    }
    
    p.expect(TokenKind::RBrace);
    return inst;
}

// Parse: enum Name { Value1, Value2, Value3 }
constexpr EnumDecl parse_enum(Parser& p) {
    EnumDecl decl;
    
    p.expect(TokenKind::Enum);
    decl.name = p.expect(TokenKind::Identifier).text;
    p.expect(TokenKind::LBrace);
    
    while (!p.check(TokenKind::RBrace) && !p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::Identifier)) {
            if (decl.value_count < decl.values.size()) {
                decl.values[decl.value_count++].name = p.current.text;
                p.advance();
                
                // Optional comma
                p.accept(TokenKind::Comma);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    p.expect(TokenKind::RBrace);
    return decl;
}

// Parse: event eventName(param1: type1, param2: type2)
constexpr EventDecl parse_event(Parser& p) {
    EventDecl decl;
    
    p.expect(TokenKind::Event);
    decl.name = p.expect(TokenKind::Identifier).text;
    p.expect(TokenKind::LParen);
    
    while (!p.check(TokenKind::RParen) && !p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::Identifier)) {
            if (decl.param_count < decl.params.size()) {
                EventParam param;
                param.name = p.current.text;
                p.advance();
                p.expect(TokenKind::Colon);
                param.type = parse_type_ref(p);
                decl.params[decl.param_count++] = param;
                
                // Optional comma
                p.accept(TokenKind::Comma);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    p.expect(TokenKind::RParen);
    return decl;
}

// Parse: when (condition) { assignments }
constexpr WhenStmt parse_when(Parser& p) {
    WhenStmt stmt;
    
    p.expect(TokenKind::When);
    p.expect(TokenKind::LParen);
    
    // For now, just grab everything until the closing paren as the condition
    size_t start = p.lexer.pos - p.current.text.size();
    int paren_depth = 1;
    while (paren_depth > 0 && !p.check(TokenKind::EndOfFile)) {
        p.advance();
        if (p.check(TokenKind::LParen)) paren_depth++;
        if (p.check(TokenKind::RParen)) {
            paren_depth--;
            if (paren_depth == 0) {
                // Found the closing paren
                stmt.condition = p.lexer.src.substr(start, p.lexer.pos - start - p.current.text.size());
                break;
            }
        }
    }
    p.expect(TokenKind::RParen);
    
    p.expect(TokenKind::LBrace);
    
    while (!p.check(TokenKind::RBrace) && !p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::Identifier)) {
            if (stmt.assignment_count < stmt.assignments.size()) {
                stmt.assignments[stmt.assignment_count++] = parse_property_assignment(p);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    p.expect(TokenKind::RBrace);
    return stmt;
}

// Parse: animate { property: value, duration: 300, easing: "ease_in" }
constexpr AnimationDecl parse_animate(Parser& p) {
    AnimationDecl anim;
    
    p.expect(TokenKind::Animate);
    p.expect(TokenKind::LBrace);
    
    while (!p.check(TokenKind::RBrace) && !p.check(TokenKind::EndOfFile)) {
        if (!p.check(TokenKind::Identifier)) {
            p.advance();
            continue;
        }
        
        auto prop_name = p.current.text;
        p.advance();
        p.expect(TokenKind::Colon);
        
        if (prop_name == "property") {
            // Target property to animate
            if (p.check(TokenKind::Identifier)) {
                anim.target_property = p.current.text;
                p.advance();
            }
        } else if (prop_name == "from") {
            // Start value
            anim.start_value = parse_value(p);
        } else if (prop_name == "to") {
            // End value
            anim.end_value = parse_value(p);
        } else if (prop_name == "duration") {
            // Duration in milliseconds
            if (p.check(TokenKind::IntegerLiteral)) {
                // Simple integer parsing
                int val = 0;
                for (char c : p.current.text) {
                    if (c >= '0' && c <= '9') {
                        val = val * 10 + (c - '0');
                    }
                }
                anim.duration_ms = val;
                p.advance();
            }
        } else if (prop_name == "easing") {
            // Easing function
            if (p.check(TokenKind::StringLiteral)) {
                anim.easing = p.current.text;
                p.advance();
            } else if (p.check(TokenKind::Identifier)) {
                anim.easing = p.current.text;
                p.advance();
            }
        } else if (prop_name == "delay") {
            // Delay before animation starts
            if (p.check(TokenKind::IntegerLiteral)) {
                int val = 0;
                for (char c : p.current.text) {
                    if (c >= '0' && c <= '9') {
                        val = val * 10 + (c - '0');
                    }
                }
                anim.delay_ms = val;
                p.advance();
            }
        } else if (prop_name == "repeat") {
            // Whether to repeat
            if (p.check(TokenKind::BoolLiteral)) {
                anim.repeat = p.current.text == "true";
                p.advance();
            }
        } else {
            // Unknown property, skip value
            p.advance();
        }
    }
    
    p.expect(TokenKind::RBrace);
    return anim;
}

// ============================================================================
// High-level parse functions
// ============================================================================

constexpr TypeDecl parse_type_from_source(std::string_view source) {
    Parser p(source);
    return parse_type_decl(p);
}

constexpr InstanceDecl parse_instance_from_source(std::string_view source) {
    InstanceNode storage;
    Parser p(source, &storage);
    return parse_instance(p);
}

// For parsing with externally managed storage
constexpr InstanceDecl parse_instance_with_storage(std::string_view source, InstanceNode& storage) {
    Parser p(source, &storage);
    return parse_instance(p);
}

constexpr EnumDecl parse_enum_from_source(std::string_view source) {
    Parser p(source);
    return parse_enum(p);
}

constexpr EventDecl parse_event_from_source(std::string_view source) {
    Parser p(source);
    return parse_event(p);
}

constexpr WhenStmt parse_when_from_source(std::string_view source) {
    Parser p(source);
    return parse_when(p);
}

// Parse import statement: import forma.animation
constexpr ImportDecl parse_import(Parser& p) {
    ImportDecl import;
    size_t start = p.current.pos;
    
    p.expect(TokenKind::Import);  // consume 'import'
    
    // Parse dot-separated path: forma.animation.widgets
    if (p.check(TokenKind::Identifier)) {
        size_t path_start = p.current.pos;
        size_t path_end = p.current.pos + p.current.text.size();
        p.advance();
        
        // Consume dots and identifiers
        while (p.accept(TokenKind::Dot)) {
            if (p.check(TokenKind::Identifier)) {
                path_end = p.current.pos + p.current.text.size();
                p.advance();
            }
        }
        
        import.module_path = p.lexer.src.substr(path_start, path_end - path_start);
        import.location = SourceLocation{start, path_end};
    } else {
        // Error: expected identifier after import
        import.location = SourceLocation{start, p.current.pos};
    }
    
    return import;
}

// Parse a complete document (imports, types, enums, events, instances)
template <size_t MaxTypes = 32, size_t MaxEnums = 16, size_t MaxEvents = 16, 
          size_t MaxImports = 32, size_t MaxInstances = 64, size_t MaxAssets = 64>
constexpr Document<MaxTypes, MaxEnums, MaxEvents, MaxImports, MaxInstances, MaxAssets> 
parse_document(std::string_view source) {
    Document<MaxTypes, MaxEnums, MaxEvents, MaxImports, MaxInstances, MaxAssets> doc;
    Parser p(source, &doc.instances);
    
    // Parse top-level declarations
    while (!p.check(TokenKind::EndOfFile)) {
        if (p.check(TokenKind::Import)) {
            if (doc.import_count < doc.imports.size()) {
                doc.imports[doc.import_count++] = parse_import(p);
            } else {
                // Too many imports, skip
                parse_import(p);
            }
        }
        else if (p.check(TokenKind::Class)) {
            if (doc.type_count < doc.types.size()) {
                // Capture position of 'class' keyword or type name
                size_t decl_pos = p.current.pos;
                auto type = parse_type_decl(p);
                // Add to symbol table with actual source location
                doc.symbols.add_symbol(Symbol::Kind::Type, type.name, 
                                      SourceLocation{0, 0, decl_pos, type.name.size()}, 
                                      doc.type_count);
                doc.types[doc.type_count++] = type;
            } else {
                // Too many types, skip
                parse_type_decl(p);
            }
        }
        else if (p.check(TokenKind::Enum)) {
            if (doc.enum_count < doc.enums.size()) {
                // Capture position of 'enum' keyword
                size_t decl_pos = p.current.pos;
                auto enum_decl = parse_enum(p);
                // Add to symbol table with actual source location
                doc.symbols.add_symbol(Symbol::Kind::Enum, enum_decl.name,
                                      SourceLocation{0, 0, decl_pos, enum_decl.name.size()},
                                      doc.enum_count);
                doc.enums[doc.enum_count++] = enum_decl;
            } else {
                // Too many enums, skip
                parse_enum(p);
            }
        }
        else if (p.check(TokenKind::Event)) {
            if (doc.event_count < doc.events.size()) {
                // Capture position of 'event' keyword
                size_t decl_pos = p.current.pos;
                auto event = parse_event(p);
                // Add to symbol table with actual source location
                doc.symbols.add_symbol(Symbol::Kind::Event, event.name,
                                      SourceLocation{0, 0, decl_pos, event.name.size()},
                                      doc.event_count);
                doc.events[doc.event_count++] = event;
            } else {
                // Too many events, skip
                parse_event(p);
            }
        }
        else if (p.check(TokenKind::Identifier)) {
            // Instance declaration
            doc.instances.add_instance(parse_instance(p));
        }
        else {
            // Unknown token, skip it
            p.advance();
        }
    }
    
    return doc;
}

} // namespace forma
