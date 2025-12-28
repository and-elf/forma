# Import Resolution Algorithm

## Overview

The Forma import system uses dot-notation to specify module paths that are resolved to `.fml` files on the filesystem.

## Syntax

```forma
import forma.animation
import forma.color
import myapp.components
```

## Resolution Process

### 1. Parse Import Statement

The parser extracts the dot-separated module path:
- `import forma.animation` → module_path: `"forma.animation"`
- Stored in `ImportDecl.module_path` as a string_view

### 2. Path Resolution

The module path is resolved using a list of import search directories:

```
module_path: "forma.animation"
import_paths: ["/usr/lib/forma", "./lib", "/home/user/.forma"]

Resolution steps:
1. Convert dots to path separators: "forma.animation" → "forma/animation"
2. Append .fml extension: "forma/animation.fml"
3. Search in each import path:
   - /usr/lib/forma/forma/animation.fml
   - ./lib/forma/animation.fml  ← FOUND
4. Return resolved path
```

### 3. Import Path Sources (Priority Order)

1. **Command-line flags**: `--import <path>`
   - Can be specified multiple times
   - Highest priority
   - Example: `forma --import ./lib --import /usr/local/lib/forma compile app.fml`

2. **TOML configuration**: `[imports]` section in `forma.toml`
   - Project-level configuration
   - Relative paths resolved from TOML file location
   - Example:
     ```toml
     [imports]
     paths = ["./lib", "../shared"]
     ```

3. **Default system path**: Built-in at compile time
   - `<install_prefix>/lib/forma`
   - Contains standard library (forma.animation, forma.color, forma.widgets, etc.)
   - Lowest priority

### 4. File Loading

Once resolved, the .fml file is:
1. Read from disk
2. Parsed into a Document
3. Type declarations merged into the importing document's symbol table
4. Instances (if any) are ignored - imports only pull in type definitions

## Implementation Details

### Data Structures

```cpp
// Import declaration from parser
struct ImportDecl {
    std::string_view module_path;  // "forma.animation"
    SourceLocation location;
};

// Resolved import (runtime)
struct ResolvedImport {
    std::string module_path;      // "forma.animation"
    std::string file_path;        // "/usr/lib/forma/forma/animation.fml"
    Document parsed_doc;          // Parsed content
};
```

### Resolution Function

```cpp
std::optional<std::string> resolve_import(
    std::string_view module_path,
    const std::vector<std::string>& import_paths
) {
    // Convert dots to slashes
    std::string rel_path = module_path;
    std::replace(rel_path.begin(), rel_path.end(), '.', '/');
    rel_path += ".fml";
    
    // Search in each import path
    for (const auto& base : import_paths) {
        std::filesystem::path full_path = 
            std::filesystem::path(base) / rel_path;
        
        if (std::filesystem::exists(full_path)) {
            return full_path.string();
        }
    }
    
    return std::nullopt;  // Not found
}
```

### Symbol Merging

```cpp
void merge_imports(Document& main_doc, const Document& imported_doc) {
    // Merge type declarations
    for (size_t i = 0; i < imported_doc.type_count; ++i) {
        if (main_doc.type_count < main_doc.types.size()) {
            main_doc.types[main_doc.type_count++] = imported_doc.types[i];
        }
    }
    
    // Merge enums
    for (size_t i = 0; i < imported_doc.enum_count; ++i) {
        if (main_doc.enum_count < main_doc.enums.size()) {
            main_doc.enums[main_doc.enum_count++] = imported_doc.enums[i];
        }
    }
    
    // Note: Instances are NOT imported, only type definitions
}
```

## Error Handling

### Import Not Found

```
Error: Cannot resolve import 'forma.widgets'
  --> app.fml:1:8
   |
 1 | import forma.widgets
   |        ^^^^^^^^^^^^^ not found in any import path
   |
   = note: searched in:
     - ./lib/forma/widgets.fml
     - /usr/lib/forma/forma/widgets.fml
   = help: check that the module exists or add the correct path with --import
```

### Circular Imports

```
Error: Circular import detected
  --> app.fml:1:8
   |
 1 | import components.button
   |        ^^^^^^^^^^^^^^^^^ 
   |
   = note: import chain:
     app.fml → components.button → components.base → app.fml
```

### Duplicate Imports

Duplicate imports are allowed but only processed once:
```forma
import forma.animation  // Processed
import forma.color
import forma.animation  // Skipped (already imported)
```

## Standard Library Location

The standard library is located at:
- **Development**: `<project_root>/lib/forma/`
- **Installed**: `<install_prefix>/lib/forma/`

Standard library modules:
- `forma.animation` - Animation curves and utilities
- `forma.color` - Color type and palette
- `forma.widgets` - All LVGL widget types
- `forma.layout` - Layout containers (Row, Column, Table, Grid, Flex, Stack, ScrollView)
- `forma.style` - Style definitions (future)

## Configuration Examples

### forma.toml

```toml
[imports]
# Additional import search paths
paths = [
    "./lib",           # Project-local library
    "../shared",       # Shared across projects
    "~/mylibs/forma"   # User-global library
]

# System path is always appended automatically
```

### Command Line

```bash
# Single import path
forma --import ./lib compile app.fml

# Multiple import paths
forma --import ./lib --import /opt/forma compile app.fml

# Override config with CLI
forma --import ./custom_lib compile app.fml
```

## Future Enhancements

1. **Namespace support**: `import forma.animation as anim`
2. **Selective imports**: `import forma.widgets.Button`
3. **Export control**: `export class MyButton`
4. **Package manager integration**: `import @vendor/package`
5. **Remote imports**: `import https://cdn.forma.dev/widgets`
