#pragma once

#include "ir.hpp"
#include <array>
#include <string_view>
#include <cstddef>

namespace forma::codegen {

template<size_t MaxOutput = 65536>
class CCodeGenerator {
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

    constexpr const char* map_type_to_c(const TypeRef& type) const {
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
        if (type.name == "string") return "const char*";
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

    constexpr void generate_class_instances(const auto& document) {
        // Generate global instances for classes (TypeDecl with methods)
        bool has_classes = false;
        for (size_t i = 0; i < document.type_count; ++i) {
            if (document.types[i].method_count > 0) {
                has_classes = true;
                break;
            }
        }
        
        if (!has_classes) return;
        
        append_line("/* ============================================================================");
        append_line(" * Class Definitions (Public API)");
        append_line(" * ============================================================================ */");
        append_line();
        
        // Generate struct definitions for each class
        for (size_t i = 0; i < document.type_count; ++i) {
            const auto& type = document.types[i];
            if (type.method_count == 0) continue;  // Not a class
            
            // Generate typedef struct
            append("typedef struct {\n");
            indent_level++;
            
            // Generate properties
            for (size_t j = 0; j < type.prop_count; ++j) {
                const auto& prop = type.properties[j];
                for (size_t k = 0; k < indent_level; ++k) append("    ");
                append(map_type_to_c(prop.type));
                append(" ");
                append(prop.name);
                append(";\n");
            }
            
            indent_level--;
            append("} ");
            append(type.name);
            append(";\n\n");
        }
        
        // Generate global instances
        append_line("/* Class Instances (Global) */");
        for (size_t i = 0; i < document.type_count; ++i) {
            const auto& type = document.types[i];
            if (type.method_count == 0) continue;
            
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
            append(" = {");
            
            // Initialize properties with default values
            bool first = true;
            for (size_t j = 0; j < type.prop_count; ++j) {
                if (!first) append(", ");
                first = false;
                
                append(".");
                append(type.properties[j].name);
                append(" = ");
                
                // Default initialization based on type
                if (type.properties[j].type.name == "int" || 
                    type.properties[j].type.name == "i32") {
                    append("0");
                } else if (type.properties[j].type.name == "bool") {
                    append("false");
                } else if (type.properties[j].type.name == "string") {
                    append("NULL");
                } else {
                    append("0");
                }
            }
            
            append("};\n");
        }
        append_line();
    }

public:
    constexpr CCodeGenerator() = default;

    constexpr void generate(const auto& document) {
        output_pos = 0;
        indent_level = 0;
        
        // Generate standard C header
        append_line("#include <stdint.h>");
        append_line("#include <stdbool.h>");
        append_line("#include <stddef.h>");
        append_line();
        
        // Generate class definitions and instances
        generate_class_instances(document);
        
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
