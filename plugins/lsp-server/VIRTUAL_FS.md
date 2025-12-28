# VirtualFS - In-Memory File System for Testing

A lightweight virtual filesystem implementation for testing the Forma LSP without actual file I/O.

## Features

- **In-Memory Storage**: All files stored in arrays, no disk I/O
- **Version Tracking**: Each file has a version number for LSP synchronization
- **LSP Integration**: Automatic LSP notifications on file changes
- **Simple API**: Create, read, update, delete operations
- **Compile-Time Safe**: Uses constexpr where possible

## Components

### VirtualFS

Core filesystem with basic operations:

```cpp
VirtualFS<64> fs;  // Up to 64 files

// Write/create file
fs.write_file("file:///test.fml", "Point { property x: int }", 1);

// Read file
auto content = fs.read_file("file:///test.fml");
if (content) {
    std::cout << *content << "\n";
}

// Check existence
if (fs.exists("file:///test.fml")) { ... }

// Get version
int version = fs.get_version("file:///test.fml");

// Delete file
fs.remove_file("file:///test.fml");

// List all files
auto files = fs.list_files();

// Get count
size_t count = fs.count();

// Clear all
fs.clear();
```

### VirtualWorkspace

Higher-level workspace that integrates VirtualFS with LSP:

```cpp
LSPDocumentManager<16> lsp;
VirtualWorkspace workspace(lsp);

// Initialize LSP
workspace.initialize();

// Create file (automatically opens in LSP)
workspace.create_file("file:///point.fml", 
    "Point { property x: int property y: int }");

// Update file (automatically syncs to LSP)
workspace.update_file("file:///point.fml",
    "Point { property x: int property y: int property z: int }");

// Get diagnostics
auto* doc = workspace.get_diagnostics("file:///point.fml");
if (doc) {
    std::cout << "Diagnostics: " << doc->diagnostic_count << "\n";
    for (size_t i = 0; i < doc->diagnostic_count; ++i) {
        std::cout << "  " << doc->diagnostics[i].message << "\n";
    }
}

// Delete file (automatically closes in LSP)
workspace.delete_file("file:///point.fml");

// Access underlying components
workspace.filesystem();  // Direct FS access
workspace.lsp();         // Direct LSP access
```

## Example: Testing Forward References

```cpp
LSPDocumentManager<16> lsp;
VirtualWorkspace workspace(lsp);

workspace.initialize();

// Create file where Point is used before it's defined
const char* code = R"(
Rectangle {
    property topLeft: Point
    property bottomRight: Point
}

Point {
    property x: int
    property y: int
}
)";

workspace.create_file("file:///shapes.fml", code);

auto* doc = workspace.get_diagnostics("file:///shapes.fml");
// doc->diagnostic_count == 0 (forward references work!)
```

## Example: Testing Error Detection

```cpp
LSPDocumentManager<16> lsp;
VirtualWorkspace workspace(lsp);

workspace.initialize();

// Create file with unknown type
workspace.create_file("file:///error.fml",
    "MyRect { property pos: UnknownType }");

auto* doc = workspace.get_diagnostics("file:///error.fml");
// doc->diagnostic_count == 1
// doc->diagnostics[0].message == "UnknownType"
// doc->diagnostics[0].code == "unknown-type"
```

## Example: Testing Generic Types

```cpp
workspace.create_file("file:///test.fml",
    "Points { property data: Forma.Array(int, 10) }");

auto* doc = workspace.get_diagnostics("file:///test.fml");
// doc->diagnostic_count == 0 (correct parameters)

workspace.update_file("file:///test.fml",
    "Points { property data: Forma.Array(int) }");

doc = workspace.get_diagnostics("file:///test.fml");
// doc->diagnostic_count == 1 (missing size parameter)
```

## Running Tests

```bash
# Using the script
chmod +x run_vfs_test.sh
./run_vfs_test.sh

# Or directly
g++ -std=c++20 -I. test_virtual_fs.cpp -o test_vfs
./test_vfs

# Or with CMake
./build.sh
./build/forma_vfs_test
```

## Test Coverage

The test suite includes:

1. **Basic Operations**: Create, read, update, delete files
2. **Workspace Integration**: LSP document lifecycle management
3. **Forward References**: Types used before definition
4. **Generic Types**: Forma.Array validation
5. **Enums and Events**: Multi-declaration type support
6. **Unknown Type Diagnostics**: Single and multiple unknown types
7. **Unknown Base Type**: Invalid inheritance detection
8. **Generic Type Errors**: Wrong parameter counts and types
9. **Multiple Errors**: Multiple diagnostics per file
10. **Severity Levels**: Error severity validation
11. **Error Recovery**: Fix errors and verify diagnostics clear

### Diagnostic Test Examples

**Unknown Type Detection**:
```cpp
// Single unknown type
workspace.create_file("file:///test.fml",
    "Widget { property data: UnknownType }");
// → 1 diagnostic: "UnknownType" with code "unknown-type"

// Multiple unknown types
workspace.create_file("file:///test.fml",
    "Widget { property a: Type1 property b: Type2 }");
// → 2 diagnostics for Type1 and Type2
```

**Unknown Base Type**:
```cpp
workspace.create_file("file:///test.fml",
    "MyWidget: UnknownBase { property x: int }");
// → 1 diagnostic: "UnknownBase" with code "unknown-type"
```

**Generic Type Parameter Errors**:
```cpp
// Too few parameters
workspace.create_file("file:///test.fml",
    "Widget { property items: Forma.Array(int) }");
// → 1 diagnostic: "Forma.Array requires 2 parameters"

// Wrong parameter type
workspace.create_file("file:///test.fml",
    "Widget { property items: Forma.Array(int, string) }");
// → 1 diagnostic: "Second parameter must be an integer"
```

**Multiple Errors in One File**:
```cpp
workspace.create_file("file:///test.fml", R"(
Widget: UnknownBase {
    property a: UnknownType1
    property b: UnknownType2
    property items: Forma.Array(int)
}
event onUpdate(param: UnknownParamType)
)");
// → 5 diagnostics total
```

**Error Recovery**:
```cpp
// Create with error
workspace.create_file("file:///test.fml",
    "Widget { property data: UnknownType }");
// → 1 diagnostic

// Fix the error
workspace.update_file("file:///test.fml",
    "Widget { property data: int }");
// → 0 diagnostics (cleared!)
```

## Implementation Notes

- Files are stored in `std::array` for compile-time sizing
- URIs are used as file identifiers (e.g., "file:///test.fml")
- Version numbers increment automatically on updates
- LSP notifications are sent automatically on file operations
- Deleted files are marked inactive but remain in the array
- Maximum file count is configurable via template parameter

## Benefits for Testing

- **No File I/O**: Tests run entirely in memory, faster and isolated
- **Deterministic**: No filesystem side effects or cleanup needed
- **Simple**: Easy to set up test scenarios
- **Type Safe**: Compile-time checked operations
- **LSP Integration**: Full document lifecycle testing
