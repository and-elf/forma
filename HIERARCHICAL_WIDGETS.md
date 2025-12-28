# Hierarchical Widget Support

Forma now fully supports hierarchical widget trees with proper parent-child relationships.

## How It Works

### Parsing
When parsing `.forma` files, nested widget declarations create hierarchical structures:

```forma
Container {
    Button {
        Label {
            text: "Click Me"
        }
    }
}
```

The parser stores instances in a flat array using depth-first traversal. Each parent instance maintains an array of child indices (`child_indices[]`) pointing to its direct children in the flat array.

### IR Structure

```cpp
struct InstanceDecl {
    std::array<size_t, 16> child_indices{};  // Indices of direct children
    size_t child_count = 0;                   // Number of children
};
```

### Code Generation

The LVGL renderer:
1. Identifies root instances (those not referenced as children)
2. Recursively generates code for each root and its descendants
3. Creates child widgets with correct parent references:
   ```c
   container = lv_obj_create(lv_scr_act());     // Root
   button = lv_btn_create(container);            // Child of container
   label = lv_label_create(button);              // Child of button
   ```

## Tokenizer Improvements

The tokenizer now properly handles both line (`//`) and block (`/* */`) comments, which are skipped during parsing.

## Examples

See:
- `examples/simple_hierarchy.forma` - Basic parent-child relationship
- `examples/hierarchy_test.forma` - Complex nested structure with multiple levels
- `examples/layout_example.forma` - Layout widgets with nested children
