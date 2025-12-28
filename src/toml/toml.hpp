#pragma once

#include <string_view>
#include <array>
#include <optional>
#include <cstdint>

namespace forma::toml {

// TOML value types
enum class ValueType {
    String,
    Integer,
    Float,
    Boolean,
    Array,
    Table,
    None
};

// Forward declare for array support
struct Value;

// Array of TOML values
template<size_t MaxElements = 16>
struct Array {
    std::array<std::string_view, MaxElements> elements{};
    size_t count = 0;
    
    constexpr void add(std::string_view elem) {
        if (count < MaxElements) {
            elements[count++] = elem;
        }
    }
};

// TOML value
struct Value {
    ValueType type = ValueType::None;
    std::string_view string_value;
    int64_t int_value = 0;
    double float_value = 0.0;
    bool bool_value = false;
    size_t array_index = 0;  // Index into Document.arrays[] for array values
    
    constexpr Value() = default;
    constexpr Value(std::string_view s) : type(ValueType::String), string_value(s) {}
    constexpr Value(int64_t i) : type(ValueType::Integer), int_value(i) {}
    constexpr Value(double f) : type(ValueType::Float), float_value(f) {}
    constexpr Value(bool b) : type(ValueType::Boolean), bool_value(b) {}
};

// Key-value pair
struct KeyValue {
    std::string_view key;
    Value value;
    
    constexpr KeyValue() = default;
    constexpr KeyValue(std::string_view k, Value v) : key(k), value(v) {}
};

// TOML table/section
template<size_t MaxEntries = 32>
struct Table {
    std::string_view name;
    std::array<KeyValue, MaxEntries> entries{};
    size_t entry_count = 0;
    
    constexpr Table() = default;
    constexpr Table(std::string_view n) : name(n) {}
    
    constexpr void add(std::string_view key, Value value) {
        if (entry_count < MaxEntries) {
            entries[entry_count++] = KeyValue{key, value};
        }
    }
    
    constexpr const Value* get(std::string_view key) const {
        for (size_t i = 0; i < entry_count; ++i) {
            if (entries[i].key == key) {
                return &entries[i].value;
            }
        }
        return nullptr;
    }
    
    constexpr std::optional<std::string_view> get_string(std::string_view key) const {
        const Value* v = get(key);
        if (v && v->type == ValueType::String) {
            return v->string_value;
        }
        return std::nullopt;
    }
    
    constexpr std::optional<int64_t> get_int(std::string_view key) const {
        const Value* v = get(key);
        if (v && v->type == ValueType::Integer) {
            return v->int_value;
        }
        return std::nullopt;
    }
    
    constexpr std::optional<bool> get_bool(std::string_view key) const {
        const Value* v = get(key);
        if (v && v->type == ValueType::Boolean) {
            return v->bool_value;
        }
        return std::nullopt;
    }
    
    constexpr size_t get_array_index(std::string_view key) const {
        const Value* v = get(key);
        if (v && v->type == ValueType::Array) {
            return v->array_index;
        }
        return static_cast<size_t>(-1);
    }
};

// TOML document
template<size_t MaxTables = 16>
struct Document {
    std::array<Table<32>, MaxTables> tables{};
    size_t table_count = 0;
    Table<32> root;  // Root table for top-level key-value pairs
    std::array<Array<16>, 32> arrays{};  // Storage for array values
    size_t array_count = 0;
    
    constexpr Document() = default;
    
    constexpr Table<32>* get_table(std::string_view name) {
        if (name.empty()) return &root;
        
        for (size_t i = 0; i < table_count; ++i) {
            if (tables[i].name == name) {
                return &tables[i];
            }
        }
        return nullptr;
    }
    
    constexpr const Table<32>* get_table(std::string_view name) const {
        if (name.empty()) return &root;
        
        for (size_t i = 0; i < table_count; ++i) {
            if (tables[i].name == name) {
                return &tables[i];
            }
        }
        return nullptr;
    }
    
    constexpr Table<32>* add_table(std::string_view name) {
        if (table_count < MaxTables) {
            tables[table_count] = Table<32>(name);
            return &tables[table_count++];
        }
        return nullptr;
    }
};

// Simple TOML parser
class Parser {
    std::string_view input;
    size_t pos = 0;
    
public:
    constexpr Parser(std::string_view src) : input(src) {}
    
    constexpr char peek() const {
        return pos < input.size() ? input[pos] : '\0';
    }
    
    constexpr char advance() {
        return pos < input.size() ? input[pos++] : '\0';
    }
    
    constexpr void skip_whitespace() {
        while (pos < input.size() && (input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\r' || input[pos] == '\n')) {
            pos++;
        }
    }
    
    constexpr void skip_line() {
        while (pos < input.size() && input[pos] != '\n') {
            pos++;
        }
        if (pos < input.size() && input[pos] == '\n') {
            pos++; // Skip the newline
        }
    }
    
    constexpr bool is_digit(char c) const {
        return c >= '0' && c <= '9';
    }
    
    constexpr bool is_alpha(char c) const {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
    }
    
    constexpr bool is_key_char(char c) const {
        return is_alpha(c) || is_digit(c) || c == '_' || c == '-' || c == '.';
    }
    
    // Parse a bare key or quoted string key
    constexpr std::string_view parse_key() {
        skip_whitespace();
        
        if (peek() == '"') {
            return parse_string();
        }
        
        size_t start = pos;
        while (is_key_char(peek())) {
            advance();
        }
        return input.substr(start, pos - start);
    }
    
    // Parse a string value (quoted)
    constexpr std::string_view parse_string() {
        if (peek() != '"') return "";
        advance(); // Skip opening quote
        
        size_t start = pos;
        while (peek() != '"' && peek() != '\0') {
            advance();
        }
        
        std::string_view result = input.substr(start, pos - start);
        if (peek() == '"') advance(); // Skip closing quote
        return result;
    }
    
    // Parse an integer
    constexpr int64_t parse_int() {
        skip_whitespace();
        
        int64_t result = 0;
        bool negative = false;
        
        if (peek() == '-') {
            negative = true;
            advance();
        } else if (peek() == '+') {
            advance();
        }
        
        while (is_digit(peek())) {
            result = result * 10 + (peek() - '0');
            advance();
        }
        
        return negative ? -result : result;
    }
    
    // Parse a boolean
    constexpr bool parse_bool() {
        skip_whitespace();
        
        if (input.substr(pos, 4) == "true") {
            pos += 4;
            return true;
        }
        if (input.substr(pos, 5) == "false") {
            pos += 5;
            return false;
        }
        return false;
    }
    
    // Parse an array ["a", "b", "c"]
    template<size_t MaxTables>
    constexpr size_t parse_array(Document<MaxTables>& doc) {
        if (peek() != '[') return static_cast<size_t>(-1);
        advance();  // Skip [
        
        if (doc.array_count >= doc.arrays.size()) return static_cast<size_t>(-1);
        size_t arr_idx = doc.array_count++;
        Array<16>& arr = doc.arrays[arr_idx];
        
        skip_whitespace();
        
        while (peek() != ']' && peek() != '\0') {
            skip_whitespace();
            
            // Parse string element
            if (peek() == '"') {
                arr.add(parse_string());
            }
            
            skip_whitespace();
            
            // Skip comma
            if (peek() == ',') {
                advance();
            }
            
            skip_whitespace();
        }
        
        if (peek() == ']') advance();  // Skip ]
        
        return arr_idx;
    }
    
    // Parse a value
    template<size_t MaxTables>
    constexpr Value parse_value(Document<MaxTables>& doc) {
        skip_whitespace();
        
        char c = peek();
        
        // Array
        if (c == '[') {
            size_t arr_idx = parse_array(doc);
            Value v;
            v.type = ValueType::Array;
            v.array_index = arr_idx;
            return v;
        }
        
        // String
        if (c == '"') {
            return Value(parse_string());
        }
        
        // Boolean
        if (input.substr(pos, 4) == "true" || input.substr(pos, 5) == "false") {
            return Value(parse_bool());
        }
        
        // Integer (simplified - no float support for now)
        if (is_digit(c) || c == '-' || c == '+') {
            return Value(parse_int());
        }
        
        return Value();
    }
    
    // Parse a table header [section]
    constexpr std::string_view parse_table_header() {
        if (peek() != '[') return "";
        advance(); // Skip [
        
        skip_whitespace();
        size_t start = pos;
        
        while (peek() != ']' && peek() != '\0') {
            advance();
        }
        
        std::string_view name = input.substr(start, pos - start);
        
        if (peek() == ']') advance(); // Skip ]
        skip_line(); // Skip to next line
        
        return name;
    }
    
    // Parse entire document
    template<size_t MaxTables = 16>
    constexpr Document<MaxTables> parse_document() {
        Document<MaxTables> doc;
        Table<32>* current_table = &doc.root;
        
        while (pos < input.size()) {
            size_t last_pos = pos;  // Track position to detect infinite loops
            skip_whitespace();
            
            if (peek() == '\0') break;
            
            // Comment
            if (peek() == '#') {
                skip_line();
                continue;
            }
            
            // Empty line
            if (peek() == '\n') {
                advance();
                continue;
            }
            
            // Table header
            if (peek() == '[') {
                std::string_view table_name = parse_table_header();
                current_table = doc.add_table(table_name);
                if (!current_table) current_table = &doc.root;
                continue;
            }
            
            // Key-value pair
            std::string_view key = parse_key();
            skip_whitespace();
            
            if (peek() == '=') {
                advance(); // Skip =
                Value value = parse_value(doc);
                if (current_table) {
                    current_table->add(key, value);
                }
            }
            
            skip_line();
            
            // Safety check: if position hasn't advanced, force advance to prevent infinite loop
            if (pos == last_pos && pos < input.size()) {
                pos++;
            }
        }
        
        return doc;
    }
};

// Helper function to parse TOML
template<size_t MaxTables = 16>
constexpr Document<MaxTables> parse(std::string_view input) {
    Parser parser(input);
    return parser.parse_document<MaxTables>();
}

} // namespace forma::toml
