#pragma once

#include "ir.hpp"
#include <array>
#include <string_view>
#include <cstddef>

namespace forma::codegen {

template<size_t MaxOutput = 65536>
class CppCodeGenerator {
    std::array<char, MaxOutput> output_buffer{};
    size_t output_pos = 0;
    size_t indent_level = 0;

    constexpr void append(const char* str) {
        while (*str && output_pos < MaxOutput - 1) {
            output_buffer[output_pos++] = *str++;
        }
    }

    constexpr void append(std::string_view str) {
        for (char c : str) {
            if (output_pos < MaxOutput - 1) {
                output_buffer[output_pos++] = c;
            }
        }
    }

    constexpr void append_line(const char* str = "") {
        append(str);
        if (output_pos < MaxOutput - 1) {
            output_buffer[output_pos++] = '\n';
        }
    }

    constexpr void append_indent() {
        for (size_t i = 0; i < indent_level; ++i) {
            append("    ");
        }
    }

    constexpr const char* map_type_to_cpp(const TypeRef& type) const {
        if (type.name == "int" || type.name == "i32") return "int32_t";
        if (type.name == "i64") return "int64_t";
        if (type.name == "i16") return "int16_t";
        if (type.name == "i8") return "int8_t";
        if (type.name == "u32") return "uint32_t";
        if (type.name == "u64") return "uint64_t";
        if (type.name == "u16") return "uint16_t";
        if (type.name == "u8") return "uint8_t";
        if (type.name == "f32" || type.name == "float") return "float";
        if (type.name == "f64" || type.name == "double") return "double";
        if (type.name == "bool") return "bool";
        if (type.name == "string") return "std::string";
        if (type.name == "void") return "void";
        
        // Unknown type, use as-is (might be a custom type)
        static char buffer[64];
        size_t len = type.name.size() < 63 ? type.name.size() : 63;
        for (size_t i = 0; i < len; ++i) {
            buffer[i] = type.name[i];
        }
        buffer[len] = '\0';
        return buffer;
    }

    constexpr const char* get_default_value(std::string_view type_name) const {
        if (type_name == "int" || type_name == "i32" || 
            type_name == "i64" || type_name == "i16" || type_name == "i8" ||
            type_name == "u32" || type_name == "u64" || 
            type_name == "u16" || type_name == "u8") {
            return "0";
        }
        if (type_name == "bool") return "false";
        if (type_name == "float" || type_name == "f32" || 
            type_name == "double" || type_name == "f64") {
            return "0.0";
        }
        if (type_name == "string") return "\"\"";
        return "{}";
    }

    constexpr void generate_class_definitions(const auto& document) {
        // Generate class definitions for classes (TypeDecl with methods)
        bool has_classes = false;
        for (size_t i = 0; i < document.type_count; ++i) {
            if (document.types[i].method_count > 0) {
                has_classes = true;
                break;
            }
        }
        
        if (!has_classes) return;
        
        append_line("// ============================================================================");
        append_line("// Class Definitions");
        append_line("// ============================================================================");
        append_line();
        
        // Generate class definitions
        for (size_t i = 0; i < document.type_count; ++i) {
            const auto& type = document.types[i];
            if (type.method_count == 0) continue;  // Not a class
            
            // Class declaration
            append("class ");
            append(type.name);
            
            // Inheritance
            if (type.base_type.size() > 0) {
                append(" : public ");
                append(type.base_type);
            }
            
            append(" {\n");
            append_line("public:");
            indent_level++;
            
            // Default constructor
            append_indent();
            append(type.name);
            append("()");
            
            // Member initializer list
            if (type.prop_count > 0) {
                append("\n");
                append_indent();
                append("    : ");
                for (size_t j = 0; j < type.prop_count; ++j) {
                    if (j > 0) {
                        append(",\n");
                        append_indent();
                        append("      ");
                    }
                    append(type.properties[j].name);
                    append("(");
                    append(get_default_value(type.properties[j].type.name));
                    append(")");
                }
                append("\n");
                append_indent();
                append("{}");
            } else {
                append(" = default;");
            }
            append("\n\n");
            
            // Method declarations
            for (size_t j = 0; j < type.method_count; ++j) {
                const auto& method = type.methods[j];
                
                append_indent();
                
                // Return type
                if (method.return_type.name.empty() || method.return_type.name == "void") {
                    append("void");
                } else {
                    append(map_type_to_cpp(method.return_type));
                }
                
                append(" ");
                append(method.name);
                append("(");
                
                // Parameters
                for (size_t k = 0; k < method.param_count; ++k) {
                    if (k > 0) append(", ");
                    append(map_type_to_cpp(method.params[k].type));
                    append(" ");
                    append(method.params[k].name);
                }
                
                append(");\n");
            }
            
            if (type.prop_count > 0) {
                append("\n");
                append_line("private:");
                indent_level++;
                
                // Member variables
                for (size_t j = 0; j < type.prop_count; ++j) {
                    const auto& prop = type.properties[j];
                    append_indent();
                    append(map_type_to_cpp(prop.type));
                    append(" ");
                    append(prop.name);
                    append(";\n");
                }
                
                indent_level--;
            }
            
            indent_level--;
            append_line("};");
            append_line();
        }
    }

    constexpr void generate_global_instances(const auto& document) {
        // Generate global instances
        bool has_instances = false;
        for (size_t i = 0; i < document.type_count; ++i) {
            if (document.types[i].method_count > 0) {
                has_instances = true;
                break;
            }
        }
        
        if (!has_instances) return;
        
        append_line("// ============================================================================");
        append_line("// Global Instances");
        append_line("// ============================================================================");
        append_line();
        
        for (size_t i = 0; i < document.type_count; ++i) {
            const auto& type = document.types[i];
            if (type.method_count == 0) continue;
            
            // Inline variable (C++17)
            append("inline ");
            append(type.name);
            append(" ");
            
            // Convert type name to lowercase for instance name
            for (size_t j = 0; j < type.name.size() && output_pos < MaxOutput - 1; ++j) {
                char c = type.name[j];
                if (c >= 'A' && c <= 'Z') {
                    output_buffer[output_pos++] = c + ('a' - 'A');
                } else {
                    output_buffer[output_pos++] = c;
                }
            }
            append(";\n");
        }
        append_line();
    }

public:
    constexpr CppCodeGenerator() = default;

    constexpr void generate(const auto& document) {
        output_pos = 0;
        indent_level = 0;
        
        // Generate header guard
        append_line("#pragma once");
        append_line();
        
        // Generate standard C++ headers
        append_line("#include <cstdint>");
        append_line("#include <string>");
        append_line();
        
        // Generate class definitions
        generate_class_definitions(document);
        
        // Generate global instances
        generate_global_instances(document);
        
        // Null terminate
        if (output_pos < MaxOutput) {
            output_buffer[output_pos] = '\0';
        }
    }

    constexpr std::string_view get_output() const {
        return std::string_view(output_buffer.data(), output_pos);
    }

    constexpr void reset() {
        output_pos = 0;
        indent_level = 0;
    }
};

} // namespace forma::codegen
