#pragma once
#include <array>
#include <string_view>
#include "../tokenizer/forma.hpp"
#include "diagnostics.hpp"

namespace forma {

// ============================================================================
// IR Type Structures
// ============================================================================

// Type parameter for generic types
struct TypeParam {
    enum class Kind {
        Type,      // A type name like 'int' or 'string'
        Integer    // An integer literal like '10'
    } kind;
    
    std::string_view value;
};

// Type reference - can be simple (int, string) or generic (Forma.Array(int, 10))
struct TypeRef {
    std::string_view name;  // e.g., "int" or "Forma.Array"
    std::array<TypeParam, 4> params{};
    size_t param_count = 0;
    
    constexpr bool is_generic() const {
        return param_count > 0;
    }
    
    // For simple types, just use name
    constexpr TypeRef() = default;
    constexpr TypeRef(std::string_view n) : name(n), param_count(0) {}
};

struct PropertyDecl {
    std::string_view name;
    TypeRef type;
    bool reactive = false;
    
    constexpr PropertyDecl() = default;
};

struct MethodParam {
    std::string_view name;
    TypeRef type;
};

struct MethodDecl {
    std::string_view name;
    TypeRef return_type;  // void for no return
    std::array<MethodParam, 8> params{};
    size_t param_count = 0;
};

struct TypeDecl {
    std::string_view name;
    std::string_view base_type;  // For inheritance: MyRect: Rectangle {}
    std::array<PropertyDecl, 8> properties{};
    size_t prop_count = 0;
    std::array<MethodDecl, 8> methods{};
    size_t method_count = 0;
    
    // Capabilities required by this type (e.g., @requires(widgets, animation))
    std::array<std::string_view, 8> required_capabilities{};
    size_t required_capabilities_count = 0;
};

// Enum value
struct EnumValue {
    std::string_view name;
};

// Enum declaration: enum Name { Value1, Value2, Value3 }
struct EnumDecl {
    std::string_view name;
    std::array<EnumValue, 16> values{};
    size_t value_count = 0;
};

// Event parameter
struct EventParam {
    std::string_view name;
    TypeRef type;
};

// Event declaration: event onClicked(x: int, y: int)
struct EventDecl {
    std::string_view name;
    std::array<EventParam, 8> params{};
    size_t param_count = 0;
};

// Value for property assignments
struct Value {
    enum class Kind {
        Integer,
        Float,
        String,
        Bool,
        Identifier,
        URI  // For asset references like forma://assets/image.png
    } kind;
    
    std::string_view text;
    
    // Check if this is a forma:// URI
    constexpr bool is_forma_uri() const {
        if (kind != Kind::URI && kind != Kind::String) return false;
        return text.size() >= 8 && 
               text[0] == 'f' && text[1] == 'o' && text[2] == 'r' && text[3] == 'm' &&
               text[4] == 'a' && text[5] == ':' && text[6] == '/' && text[7] == '/';
    }
    
    // Get the path part of forma://path
    constexpr std::string_view get_uri_path() const {
        if (!is_forma_uri()) return "";
        return text.substr(8); // Skip "forma://"
    }
};

struct PropertyAssignment {
    std::string_view name;
    Value value;
    Value preview_value;  // For: value or preview{...}
    bool has_preview = false;
    
    // Helper to get value type as string for diagnostics
    constexpr std::string_view value_type_string() const {
        switch (value.kind) {
            case Value::Kind::Integer: return "int";
            case Value::Kind::Float: return "float";
            case Value::Kind::String: return "string";
            case Value::Kind::Bool: return "bool";
            case Value::Kind::Identifier: return "identifier";
            case Value::Kind::URI: return "uri";
            default: return "unknown";
        }
    }
};

// When statement for reactive programming (uses PropertyAssignment)
struct WhenStmt {
    std::string_view condition;  // For now, just store as string_view
    std::array<PropertyAssignment, 8> assignments{};
    size_t assignment_count = 0;
};

// Animation declaration for animating properties
struct AnimationDecl {
    std::string_view target_property;  // Property to animate (e.g., "x", "y", "opacity")
    Value start_value;                  // Starting value
    Value end_value;                    // Ending value
    int duration_ms = 0;                // Duration in milliseconds
    std::string_view easing;            // Easing function (e.g., "linear", "ease_in", "bounce")
    int delay_ms = 0;                   // Delay before starting in milliseconds
    bool repeat = false;                // Whether to repeat infinitely
};

// Forward declare for InstanceNode
struct InstanceNode;

// InstanceDecl is a lightweight view into the instance tree
struct InstanceDecl {
    std::string_view type_name;
    std::array<PropertyAssignment, forma::limits::max_properties> properties{};
    size_t prop_count = 0;
    std::array<WhenStmt, 8> when_stmts{};  // Event handlers
    size_t when_count = 0;
    std::array<AnimationDecl, 8> animations{};  // Animations for this instance
    size_t animation_count = 0;
    // Children are stored as indices into a flat array (in InstanceNode)
    std::array<size_t, 16> child_indices{};
    size_t child_count = 0;
};

// Storage for instance tree - uses flat array to avoid recursive types
struct InstanceNode {
    static constexpr size_t MAX_INSTANCES = 64;
    std::array<InstanceDecl, MAX_INSTANCES> instances{};
    size_t count = 0;
    
    constexpr size_t add_instance(const InstanceDecl& inst) {
        if (count < MAX_INSTANCES) {
            instances[count] = inst;
            return count++;
        }
        return 0; // Error case
    }
    
    constexpr const InstanceDecl& get(size_t idx) const {
        return instances[idx];
    }
};

// Import declaration
struct ImportDecl {
    std::string_view module_path;  // e.g., "std.ui" or "./MyComponent.fml"
    SourceLocation location;
};

// Asset declaration for bundled resources
struct AssetDecl {
    enum class Type {
        Image,
        Font,
        Binary
    } type;
    
    std::string_view uri;          // forma://assets/logo.png
    std::string_view file_path;    // Resolved file path from project root
    std::string_view symbol_name;  // Generated C symbol name (e.g., asset_logo_png)
    size_t file_size = 0;          // File size in bytes (populated during bundling)
};

// ============================================================================
// Symbol Table for Type Resolution
// ============================================================================

struct Symbol {
    enum class Kind {
        Type,      // TypeDecl
        Enum,      // EnumDecl
        Event,     // EventDecl
        Property   // PropertyDecl
    } kind;
    
    std::string_view name;
    SourceLocation location;
    size_t decl_index = 0;  // Index into the appropriate declaration array
};

template <size_t MaxSymbols = 128>
struct SymbolTable {
    std::array<Symbol, MaxSymbols> symbols{};
    size_t count = 0;
    
    constexpr void add_symbol(Symbol::Kind kind, std::string_view name, 
                             SourceLocation loc, size_t decl_idx = 0) {
        if (count < MaxSymbols) {
            symbols[count++] = Symbol{kind, name, loc, decl_idx};
        }
    }
    
    constexpr const Symbol* find(std::string_view name) const {
        for (size_t i = 0; i < count; ++i) {
            if (symbols[i].name == name) {
                return &symbols[i];
            }
        }
        return nullptr;
    }
    
    constexpr bool exists(std::string_view name) const {
        return find(name) != nullptr;
    }
};

// Document represents a complete .fml file with all declarations
template <size_t MaxTypes = 32, size_t MaxEnums = 16, size_t MaxEvents = 16, 
          size_t MaxImports = 32, size_t MaxInstances = 64, size_t MaxAssets = 64>
struct Document {
    std::array<TypeDecl, MaxTypes> types{};
    size_t type_count = 0;
    
    std::array<EnumDecl, MaxEnums> enums{};
    size_t enum_count = 0;
    
    std::array<EventDecl, MaxEvents> events{};
    size_t event_count = 0;
    
    std::array<ImportDecl, MaxImports> imports{};
    size_t import_count = 0;
    
    std::array<AssetDecl, MaxAssets> assets{};
    size_t asset_count = 0;
    
    InstanceNode instances;
    
    SymbolTable<128> symbols;
};

} // namespace forma
