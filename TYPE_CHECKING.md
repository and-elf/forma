# Property Type Checking

Forma now includes comprehensive compile-time property type checking to catch type errors early.

## Features

### Type Validation
The semantic analyzer validates that:
1. **Properties exist** - Properties assigned must be declared in the class
2. **Types match** - Property values must match their declared types
3. **Types are defined** - Referenced types must exist in the symbol table

### Supported Type Checks

#### Built-in Types
- `int` - Integer literals
- `float` - Float and integer literals (implicit conversion)
- `string` - String literals
- `bool` - Boolean literals (`true`/`false`)

#### Type Compatibility
```forma
class Button {
    property text: string
    property x: int
    property enabled: bool
}

Button {
    text: "Hello"      // ✓ Valid: string matches string
    x: 42              // ✓ Valid: int matches int
    enabled: true      // ✓ Valid: bool matches bool
}

Button {
    text: 123          // ✗ Error: int doesn't match string
    x: "wrong"         // ✗ Error: string doesn't match int
    enabled: 42        // ✗ Error: int doesn't match bool
}
```

#### Unknown Properties
```forma
Button {
    text: "OK"
    invalid: 100       // ✗ Error: property 'invalid' not declared
}
```

## Error Codes

| Code | Description |
|------|-------------|
| `type-mismatch` | Property value type doesn't match declared type |
| `unknown-property` | Property not declared in class definition |
| `unknown-type` | Referenced type doesn't exist |
| `type-mismatch-preview` | Preview value type doesn't match declared type |

## Implementation

### Parsing Phase
- Symbol table populated during parsing
- Types, enums, and events registered automatically
- Symbols include name, kind, and location

### Analysis Phase
1. **Type Resolution** - Validate type references exist
2. **Property Lookup** - Find property declaration in class
3. **Type Checking** - Compare value type with declared type

### Data Structures

```cpp
// Symbol table entry
struct Symbol {
    enum class Kind { Type, Enum, Event, Property } kind;
    std::string_view name;
    SourceLocation location;
    size_t decl_index;
};

// Type checking validates this
struct PropertyAssignment {
    std::string_view name;
    Value value;  // Actual value with runtime type
};

// Against this declaration
struct PropertyDecl {
    std::string_view name;
    TypeRef type;  // Expected type
};
```

## Testing

Run type checking tests:
```bash
./test_type_checking examples/type_checking_test.forma  # Shows errors
./test_type_checking examples/valid_types.forma          # Passes
```

## Future Enhancements

Planned improvements:
- [ ] Generic type parameter validation
- [ ] Method signature type checking
- [ ] Return type validation
- [ ] Enum value validation
- [ ] Array element type checking
- [ ] Implicit type conversions (int → float)
