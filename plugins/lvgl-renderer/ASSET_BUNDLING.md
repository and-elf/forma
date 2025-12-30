# Asset Bundling with `forma://` URIs

Forma supports bundling assets (images, fonts, binary data) directly into the generated code using the `forma://` URI scheme.

## Overview

Assets are referenced using `forma://` URIs that are resolved relative to the project root directory. The build system collects all referenced assets and converts them to C arrays that are embedded in the generated code.

## URI Scheme

```
forma://path/to/asset.ext
```

- **Scheme**: `forma://` (required)
- **Path**: Relative path from project root
- **Extension**: Determines asset type (png, jpg, ttf, etc.)

## Supported Asset Types

### Images
Extensions: `.png`, `.jpg`, `.jpeg`, `.bmp`, `.gif`, `.svg`

Converted to LVGL image descriptors (C arrays).

### Fonts
Extensions: `.ttf`, `.otf`, `.woff`, `.woff2`

Converted to LVGL font structures.

### Binary
Any other extension

Embedded as raw byte arrays.

## Usage Example

```forma
Image {
    x: 10
    y: 10
    width: 200
    height: 150
    src: "forma://assets/logo.png"
}

Image {
    x: 10
    y: 170
    src: "forma://assets/icons/settings.png"
}
```

## Generated Code

For each asset, the LVGL renderer generates:

```c
// Asset declarations (one per asset)
extern const unsigned char asset_assets_logo_png[];
extern const unsigned int asset_assets_logo_png_size;

// Usage in code
void forma_init(void) {
    image_0 = lv_img_create(lv_scr_act());
    lv_img_set_src(image_0, asset_assets_logo_png);
}
```

## Symbol Naming

URIs are converted to valid C identifiers:
- Prefix: `asset_`
- Path separators `/` → `_`
- Special characters → `_`
- Uppercase letters → lowercase

Examples:
- `forma://assets/logo.png` → `asset_assets_logo_png`
- `forma://icons/settings.png` → `asset_icons_settings_png`
- `forma://fonts/Roboto-Regular.ttf` → `asset_fonts_roboto_regular_ttf`

## Project Structure

Recommended directory layout:

```
project/
├── forma.toml
├── src/
│   └── main.forma
└── assets/
    ├── images/
    │   ├── logo.png
    │   └── background.jpg
    ├── icons/
    │   ├── play.png
    │   └── pause.png
    └── fonts/
        └── Roboto.ttf
```

## Asset Collection

The build system automatically:
1. **Scans** all `.forma` files for `forma://` URIs
2. **Resolves** paths relative to project root
3. **Collects** unique assets (deduplicates)
4. **Converts** to C arrays
5. **Embeds** in generated code

## LVGL Integration

The `lvgl-renderer` plugin handles asset bundling:

### Image Assets
- Converts to `lv_img_dsc_t` descriptors
- Supports PNG, JPG, BMP formats
- Generates `lv_img_set_src()` calls

### Font Assets
- Converts to `lv_font_t` structures
- Generates `lv_obj_set_style_text_font()` calls

### Binary Assets
- Embedded as `const unsigned char[]`
- Accessible via symbol name

## Build Process

### Manual Steps
1. Parse `.forma` files
2. Collect assets using `AssetBundler`
3. Convert assets to C using external tools (e.g., `xxd`, `lvgl_font_conv`)
4. Generate header with asset declarations
5. Link with generated code

### Example Workflow
```bash
# Parse and collect assets
./forma_compiler main.forma --collect-assets > assets.txt

# Convert images (using LVGL tools)
for img in assets/*.png; do
    lv_img_conv $img -f true_color_alpha -o ${img%.png}.c
done

# Convert fonts
lv_font_conv --font Roboto.ttf --size 16 -o roboto_16.c

# Compile all together
gcc main.c assets/*.c -o app -llvgl
```

## Configuration

In `forma.toml`:

```toml
[assets]
# Asset search paths (in addition to project root)
paths = ["./assets", "./resources"]

# Asset conversion options
image-format = "true_color_alpha"  # LVGL color format
font-sizes = [12, 16, 20, 24]      # Font sizes to generate
compress = true                     # Compress asset data
```

## Plugin Support

Asset bundling is implemented in the `lvgl-renderer` plugin:

```cpp
// Asset declaration generation
template<typename DocType>
constexpr void generate_asset_declarations(const DocType& document);

// Symbol name generation
constexpr void generate_asset_symbol_name(std::string_view uri);
```

## Testing

Test asset bundling:
```bash
./test_assets examples/asset_test.forma
```

Output shows:
- Detected assets
- Asset types
- Generated symbols
- Generated LVGL code with asset references

## Limitations

- Assets must exist at build time
- No runtime asset loading from `forma://` (compile-time only)
- Large assets increase binary size
- Asset conversion requires external tools (LVGL font converter, image converter)

## Future Enhancements

- [ ] Automatic asset conversion during build
- [ ] Asset compression
- [ ] Asset caching
- [ ] Hot reload for development
- [ ] Asset manifest generation
- [ ] Duplicate detection and deduplication
- [ ] Asset optimization (PNG crush, etc.)
