# TOML Support in Forma

Forma includes a constexpr TOML parser for project configuration and metadata.

## Features

- **Constexpr Parsing**: All parsing happens at compile time when possible
- **Type-Safe Access**: Template-based API with optional return types
- **Zero Dependencies**: Pure C++20 implementation
- **Compile-Time Validated**: Tests run at compile time via static_assert

## Supported TOML Features

### Basic Types
- ✅ Strings: `"value"`
- ✅ Integers: `42`, `-17`
- ✅ Booleans: `true`, `false`
- ✅ Comments: `# comment`
- ✅ Tables: `[section]`

### Not Yet Supported
- ❌ Floats (parsed as integers)
- ❌ Arrays: `[1, 2, 3]`
- ❌ Inline tables: `{ key = "value" }`
- ❌ Multi-line strings
- ❌ Dates and times
- ❌ Nested tables: `[a.b.c]`

## Usage

### Basic Parsing

```cpp
#include "src/toml.hpp"

using namespace forma::toml;

constexpr auto doc = parse(R"(
name = "forma"
version = "0.1.0"
year = 2025
active = true
)");

// Access values
auto name = doc.root.get_string("name");      // Optional<string_view>
auto year = doc.root.get_int("year");         // Optional<int64_t>
auto active = doc.root.get_bool("active");    // Optional<bool>

if (name) {
    std::cout << "Name: " << *name << "\n";
}
```

### Tables/Sections

```cpp
constexpr auto doc = parse(R"(
[package]
name = "my-app"
version = "1.0.0"

[build]
optimize = true
)");

auto package = doc.get_table("package");
if (package) {
    auto name = package->get_string("name");
    // Use name...
}
```

### Project Configuration

Create a `forma.toml` file:

```toml
[package]
name = "my-forma-app"
version = "1.0.0"
entry = "main.fml"

[dependencies]
forma-std = "0.1"
forma-ui = "0.2"

[build]
optimize = true
warnings = true
```

Load it in your code:

```cpp
#include "src/toml.hpp"
#include <fstream>
#include <sstream>

std::string load_file(const char* path) {
    std::ifstream f(path);
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
}

int main() {
    std::string toml_content = load_file("forma.toml");
    auto doc = forma::toml::parse(toml_content);
    
    auto package = doc.get_table("package");
    if (package) {
        auto name = package->get_string("name");
        auto version = package->get_string("version");
        std::cout << "Building " << *name << " v" << *version << "\n";
    }
}
```

## API Reference

### Document

```cpp
template<size_t MaxTables = 16>
struct Document {
    Table<32> root;  // Top-level key-value pairs
    
    // Get a table by name
    Table<32>* get_table(std::string_view name);
    const Table<32>* get_table(std::string_view name) const;
};
```

### Table

```cpp
template<size_t MaxEntries = 32>
struct Table {
    std::string_view name;
    
    // Get value by key
    const Value* get(std::string_view key) const;
    
    // Type-specific getters (return optional)
    std::optional<std::string_view> get_string(std::string_view key) const;
    std::optional<int64_t> get_int(std::string_view key) const;
    std::optional<bool> get_bool(std::string_view key) const;
    
    // Add key-value pair
    void add(std::string_view key, Value value);
};
```

### Value

```cpp
struct Value {
    ValueType type;
    
    std::string_view string_value;
    int64_t int_value;
    bool bool_value;
    
    // Constructors
    Value(std::string_view s);  // String
    Value(int64_t i);           // Integer
    Value(bool b);              // Boolean
};
```

### Parser

```cpp
// Parse TOML string
template<size_t MaxTables = 16>
Document<MaxTables> parse(std::string_view input);
```

## Examples

### Example: forma.toml

```toml
[package]
name = "forma"
version = "0.1.0"
description = "A QML-inspired programming language"

[build]
standard = "c++20"
optimize = true

[lsp]
port = 8080
diagnostics = true
max-documents = 16

[plugins]
enabled = true
directory = "./plugins"
```

### Example: Reading Configuration

```cpp
auto config = forma::toml::parse(toml_content);

// Get LSP settings
auto lsp = config.get_table("lsp");
if (lsp) {
    int port = lsp->get_int("port").value_or(8080);
    bool diagnostics = lsp->get_bool("diagnostics").value_or(true);
    
    std::cout << "LSP server on port " << port << "\n";
}

// Get build settings
auto build = config.get_table("build");
if (build) {
    auto standard = build->get_string("standard").value_or("c++20");
    bool optimize = build->get_bool("optimize").value_or(false);
}
```

## Testing

The TOML parser includes comprehensive compile-time tests:

```bash
# All tests run at compile time via static_assert
./build.sh

# Runtime tests
g++ -std=c++20 -I. test_toml.cpp -o test_toml
./test_toml
```

### Test Coverage

- ✅ Basic key-value parsing
- ✅ Table sections
- ✅ Comments (line and inline)
- ✅ Integer values (positive, negative, zero)
- ✅ Boolean values
- ✅ Multiple tables
- ✅ Whitespace handling
- ✅ Project configuration example

## Limitations

This is a simplified TOML parser focused on the subset needed for Forma project files:

1. **No arrays**: Use multiple keys instead
2. **No floats**: Use integers or strings
3. **No nested tables**: Use flat table names
4. **No string escaping**: Literal strings only
5. **No multi-line strings**: Use single-line strings

For full TOML 1.0 support, consider using a dedicated library like `toml++`.

## Integration with Forma

TOML configuration is used for:

- **Package metadata**: name, version, description
- **Build settings**: C++ standard, optimization flags
- **LSP configuration**: port, capabilities, limits
- **Plugin settings**: enabled plugins, directories
- **Dependencies**: required libraries and versions

This allows Forma projects to have declarative configuration separate from code.
