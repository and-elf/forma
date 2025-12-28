# Forma LVGL Renderer

Generate C99 LVGL code from Forma UI definitions.

## Overview

The LVGL renderer converts Forma's intermediate representation (IR) into production-ready C99 code that uses the LVGL (Light and Versatile Graphics Library) framework. This enables embedded systems to use Forma's declarative UI syntax while leveraging LVGL's battle-tested widget library.

## Features

- **Constexpr Code Generation**: Compile-time C code generation
- **Widget Mapping**: Automatic mapping from Forma types to LVGL widgets
- **Property Translation**: Converts Forma properties to LVGL API calls
- **Enum Support**: Generates C enums from Forma enum declarations
- **Event Callbacks**: Generates static callback functions for `when` blocks
- **Animation Support**: Generates LVGL animations from `animate` blocks
- **Platform Configuration**: Support for FreeRTOS, Zephyr RTOS, Windows, and Linux
- **Class Instance Exposure**: Public API for class instances, internal widget encapsulation
- **Zero Dependencies**: Pure C99 output with only LVGL as a dependency

## Supported Widgets

| Forma Type | LVGL Widget | Description |
|------------|-------------|-------------|
| `Button` | `lv_btn` | Clickable button |
| `Label` | `lv_label` | Text display |
| `Panel` | `lv_obj` | Generic container |
| `Container` | `lv_obj` | Generic container |
| `Slider` | `lv_slider` | Value slider |
| `Switch` | `lv_switch` | Toggle switch |
| `Checkbox` | `lv_checkbox` | Checkbox |
| `Dropdown` | `lv_dropdown` | Dropdown menu |
| `TextArea` | `lv_textarea` | Multi-line text input |
| `Image` | `lv_img` | Image display |
| `Arc` | `lv_arc` | Circular arc/progress |
| `Bar` | `lv_bar` | Progress bar |
| `Spinner` | `lv_spinner` | Loading spinner |
| `List` | `lv_list` | Scrollable list |
| `Chart` | `lv_chart` | Data visualization |
| `Table` | `lv_table` | Table/grid |
| `Calendar` | `lv_calendar` | Calendar widget |
| `Keyboard` | `lv_keyboard` | Virtual keyboard |
| `Roller` | `lv_roller` | Roller selector |

## Supported Properties

| Forma Property | LVGL API | Applies To |
|---------------|----------|------------|
| `text` | `lv_label_set_text()` | Label, Button |
| `width` | `lv_obj_set_width()` | All widgets |
| `height` | `lv_obj_set_height()` | All widgets |
| `x` | `lv_obj_set_x()` | All widgets |
| `y` | `lv_obj_set_y()` | All widgets |
| `visible` | `lv_obj_set_hidden()` | All widgets |
| `enabled` | `lv_obj_set_enabled()` | All widgets |
| `value` | `lv_slider_set_value()` | Slider, Bar, Arc |
| `checked` | `lv_checkbox_set_checked()` | Checkbox |

## Animation Support

The LVGL renderer generates LVGL animation code from `animate` blocks in your Forma UI definitions.

### Animatable Properties

| Forma Property | LVGL Setter | Description |
|---------------|-------------|-------------|
| `x` | `lv_obj_set_x` | Horizontal position |
| `y` | `lv_obj_set_y` | Vertical position |
| `width` | `lv_obj_set_width` | Widget width |
| `height` | `lv_obj_set_height` | Widget height |
| `opacity` | `lv_obj_set_style_opa` | Opacity (0-255) |

### Supported Easing Functions

| Forma Easing | LVGL Path Function | Description |
|-------------|-------------------|-------------|
| `linear` | `lv_anim_path_linear` | Linear interpolation |
| `ease_in` | `lv_anim_path_ease_in` | Ease in (slow start) |
| `ease_out` | `lv_anim_path_ease_out` | Ease out (slow end) |
| `ease_in_out` | `lv_anim_path_ease_in_out` | Ease in and out |
| `overshoot` | `lv_anim_path_overshoot` | Overshoot effect |
| `bounce` | `lv_anim_path_bounce` | Bounce effect |

### Animation Example

```cpp
// Create button with animation
InstanceDecl button;
button.type_name = "Button";
button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Animate"}};
button.prop_count = 1;

// Add slide animation
AnimationDecl anim;
anim.target_property = "x";
anim.start_value = Value{Value::Kind::Integer, "0"};
anim.end_value = Value{Value::Kind::Integer, "200"};
anim.duration_ms = 1000;
anim.easing = "ease_in_out";
anim.delay_ms = 500;
anim.repeat = true;  // Infinite repeat

button.animations[0] = anim;
button.animation_count = 1;
```

### Generated Animation Code

The above example generates:

```c
void forma_init(void) {
    button_0 = lv_btn_create(lv_scr_act());
    lv_label_set_text(button_0, "Animate");
    
    lv_anim_t anim_button_0_0;
    lv_anim_init(&anim_button_0_0);
    lv_anim_set_var(&anim_button_0_0, button_0);
    lv_anim_set_values(&anim_button_0_0, 0, 200);
    lv_anim_set_time(&anim_button_0_0, 1000);
    lv_anim_set_delay(&anim_button_0_0, 500);
    lv_anim_set_repeat_count(&anim_button_0_0, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim_button_0_0, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&anim_button_0_0, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&anim_button_0_0);
}
```

## Usage

### Basic Example

```cpp
#include "lvgl_renderer.hpp"

using namespace forma::lvgl;

// Create a document
Document<8, 4, 4, 4, 16> doc;

// Add a button
InstanceDecl button;
button.type_name = "Button";
button.properties[0] = PropertyAssignment{"text", Value{Value::Kind::String, "Click"}};
button.prop_count = 1;
doc.instances.add_instance(button);

// Generate LVGL code (defaults to Linux platform)
LVGLRenderer<4096> renderer;
renderer.generate(doc);

// Output the C code
std::cout << renderer.c_str();
```

### Platform Configuration

```cpp
#include "lvgl_renderer.hpp"

using namespace forma::lvgl;

LVGLRenderer<4096> renderer;

// Configure for different platforms
renderer.set_platform(Platform::FreeRTOS);  // FreeRTOS
renderer.set_platform(Platform::ZephyrRTOS); // Zephyr RTOS
renderer.set_platform(Platform::Windows);    // Windows
renderer.set_platform(Platform::Linux);      // Linux (default)

renderer.generate(doc);
```

### From Forma Source

```cpp
#include "forma.hpp"
#include "lvgl_renderer.hpp"

const char* forma_code = R"(
Button {
    text: "Start"
    x: 10
    y: 20
    width: 100
}
)";

// Parse Forma code
Lexer lexer(forma_code);
Parser<64, 16, 16, 32> parser(lexer);
auto doc = parser.parse_document();
Platform-specific includes** (based on target platform)
   ```c
   /* Platform-specific includes */
   #include <unistd.h>              // Linux
   // or
   #include "FreeRTOS.h"            // FreeRTOS
   #include "task.h"
   // or
   #include <zephyr/kernel.h>       // Zephyr RTOS
   // or
   #include <windows.h>             // Windows
   ```

3. **Type definitions** (as comments)
   ```c
   /* type MyButton { text: string, enabled: bool } */
   ```

4. **Enum definitions**
   ```c
   typedef enum {
       Status_Idle,
       Status_Running,
       Status_Error
   } Status;
   ```

5. **Event callbacks** (internal, for `when` blocks)
   ```c
   /* Event Callbacks (Internal) */
   static void button_0_callback_0(lv_event_t* e) {
       lv_event_code_t code = lv_event_get_code(e);
       lv_obj_t* obj = lv_event_get_target(e);
       
       /* Condition-based updates */
       // ... generated property updates
   }
   ```

6. **Class instances** (public API)
   ```c
   /* Class Instances (Public API) */
   // Exposed class instances for programmatic access
   // (instances of types declared with the 'class' keyword)
   ```

7. **UI widget variables** (internal, not exposed)
   ```c
   /* UI Widgets (Internal) */
   static lv_obj_t *button_0 = NULL;
   static lv_obj_t *label_1 = NULL;
   ```

8. **UI initialization function** (`forma_init`)
   ```c
   /**
    * Initialize the Forma UI system
    * Call this once during application startup
    */
   void forma_init(void) {
       button_0 = lv_btn_create(lv_scr_act());
       lv_label_set_text(button_0, "Click Me");
       lv_obj_set_x(button_0, 10);
       lv_obj_set_y(button_0, 20);
       lv_obj_add_event_cb(button_0, button_0_callback_0, LV_EVENT_CLICKED, NULL);
   }
   ```

9. **Event loop function** (`forma_run`, platform-specific)
   ```c
   /**
    * Run the Forma UI main loop
    * This function handles platform-specific event processing
    */
   void forma_run(void) {
       /* Linux: LVGL event loop */
       while (1) {
           lv_timer_handler();
           usleep(5000);
       }, enabled: bool } */
   ```

3. **Enum definitions**
   ```c
   typedef enum {
       Status_Idle,
       Status_Running,
       Status_Error
   } Status;
   ```

4. **UI creation function**
   ```c
   void forma_ui_create(void) {
       lv_obj_t *button_0 = lv_btn_create(lv_scr_act());
       lv_label_set_text(button_0, "Click Me");
       lv_obj_set_x(button_0, 10);
       lv_obj_set_y(button_0, 20);
   }
   ```

## Complete Example

### Forma Input

```forma
enum State {
    Idle,
    Running,
    Paused
}

ControlPanel {
    property width: int
    property height: int
}
 Platform-specific includes */
#include <unistd.h>

/* UI Widgets (Internal) */
static lv_obj_t *panel_0 = NULL;
static lv_obj_t *label_1 = NULL;
static lv_obj_t *slider_2 = NULL;
static lv_obj_t *button_3 = NULL;
static lv_obj_t *button_4 = NULL;
static lv_obj_t *label_5 = NULL;

/**
 * Initialize the Forma UI system
 * Call this once during application startup
 */
void forma_init(void) {
    panel_0 = lv_obj_create(lv_scr_act());
    lv_obj_set_width(panel_0, 320);
    lv_obj_set_height(panel_0, 240);
    
    label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_1, "Device Control");
    lv_obj_set_x(label_1, 10);
    lv_obj_set_y(label_1, 10);
    
    slider_2 = lv_slider_create(lv_scr_act());
    lv_slider_set_value(slider_2, 75);
    lv_obj_set_x(slider_2, 10);
    lv_obj_set_y(slider_2, 50);
    lv_obj_set_width(slider_2, 200);
    
    button_3 = lv_btn_create(lv_scr_act());
    lv_obj_set_x(button_3, 10);
    lv_obj_set_y(button_3, 100);
    
    button_4 = lv_btn_create(lv_scr_act());
    lv_obj_set_x(button_4, 120);
    lv_obj_set_y(button_4, 100);
    
    label_5 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_5, "Ready");
    lv_obj_set_x(label_5, 10);
    lv_obj_set_y(label_5, 150);
}

/**
 * Run the Forma UI main loop
 * This function handles platform-specific event processing
 */
void forma_run(void) {
    /* Linux: LVGL event loop */
    while (1) {
        lv_timer_handler();
        usleep(5000);
    }
    text: "Ready"
    x: 10
    y: 150
}
```

### Generated C Output

```c
/**
 * Generated by Forma LVGL Renderer
 * This file contains LVGL UI code generated from .fml definitions
 */

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* Type Definitions */
/* type ControlPanel { width: int, height: int } */

typedef enum {
    State_Idle,
    State_Running,
    State_Paused
} State;

/**
 * Create and initialize the UI
 * Call this function once during application startup
 */
void forma_ui_create(void) {
    lv_obj_t *panel_0 = lv_obj_create(lv_scr_act());
    lv_obj_set_width(panel_0, 320);
    lv_obj_set_height(panel_0, 240);
    
    lv_obj_t *label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_1, "Device Control");
    lv_obj_set_x(label_1, 10);
    lv_obj_set_y(label_1, 10);
    
    lv_obj_t *slider_2 = lv_slider_create(lv_scr_act());
    lv_slider_set_value(slider_2, 75);
    lv_obj_set_x(slider_2, 10);
    lv_obj_set_y(slider_2, 50);
    lv_obj_set_width(slider_2, 200);
    
    lv_obj_t *button_3 = lv_btn_create(lv_scr_act());
    lv_obj_set_x(button_3, 10);
    lv_obj_set_y(button_3, 100);
    
    lv_obj_t *button_4 = lv_btn_create(lv_scr_act());
    lv_obj_set_x(button_4, 120);
    lv_obj_set_y(button_4, 100);
    
    lv_obj_t *label_5 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_5, "Ready");
    lv_obj_set_x(label_5, 10);
    lv_obj_set_y(label_5, 150);
}
```

## Integration with Build Systems

### CMake

```cmake
# Add LVGL library
add_subdirectory(lvgl)

# Generate C code from Forma
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ui_generated.c
    COMMAND forma_compiler --output lvgl ${CMAKE_CURRENT_SOURCE_DIR}/ui.fml
    DEPENDS ui.fml
    COMMENT "Generating LVGL code from Forma"
)

# Build your application
### Linux Example

```c
#include "lvgl.h"

// Generated by Forma
extern void forma_init(void);
extern void forma_run(void);

int main(void) {
    // Initialize LVGL
    lv_init();
    
    // Setup display driver (SDL, framebuffer, etc.)
    // ... (platform-specific code)
    
    // Initialize the Forma UI
    forma_init();
    
    // Run the event loop (never returns)
    forma_run();
    
    return 0;
}
```

### FreeRTOS Example

```c
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"

// Generated by Forma
extern void forma_init(void);
extern void forma_run(void);

void ui_task(void *pvParameters) {
    // Initialize LVGL
    lv_init();
    
    // Setup display driver
    // ... (platform-specific code)
    
    // Initialize the Forma UI
    forma_init();
    
    // Run the event loop (never returns)
    forma_run();
}

int main(void) {
    xTaskCreate(ui_task, "UI", 4096, NULL, 1, NULL);
    vTaskStartScheduler();
    
    return 0;
}
```

### Zephyr RTOS Example
set_platform(Platform platform)`
Set the target platform for code generation.

**Parameters:**
- `platform`: Target platform (`Platform::FreeRTOS`, `Platform::ZephyrRTOS`, `Platform::Windows`, or `Platform::Linux`)

**Example:**
```cpp
LVGLRenderer<8192> renderer;
renderer.set_platform(Platform::FreeRTOS);
```

#### `void generate(const Document& doc)`
Generate C code from a parsed Forma document.

**Parameters:**
- `doc`: The Forma document (IR) to convert

**Example:**
```cpp
LVGLRenderer<8192> renderer;
renderer.set_platform(Platform::Linux)
int main(void) {
    // Initialize LVGL
    lv_init();
    
    // Setup display driver
    // ... (platform-specific code)
     Actions**: Event callbacks generated but property updates need implementation
4. **No Animations**: Animation definitions not supported
5. **Basic Properties**: Advanced LVGL properties require manual coding
6. **Widget Encapsulation**: UI widgets are internal only; cannot be accessed programmatically

### Architecture Notes

**Public vs Internal API:**
- **UI Widgets** (Button, Label, Slider, etc.): Generated as `static` internal variables, not exposed
- **Class Instances**: Types declared with `class` keyword are exposed as public API
- **Programmatic UI Access**: Only class instances can be accessed from application code

This design ensures:
- UI implementation details remain encapsulated
- Class-based business logic can interact with UI through defined interfaces
- Generated code has clear separation between public API and internal implementation

### Planned Features

- [ ] Hierarchical widget trees (parent-child relationships)
- [x] Event handler generation (basic callbacks)
- [ ] Complete event property updates in callbacks
- [x] Animation definitions
- [ ] Style generation
- [ ] Theme support
- [ ] Custom widget registration
- [ ] Reactive property bindings
- [x] Layout containers (flex, grid)
- [ ] Class instance exposure and integration
int main(void) {
    // Initialize LVGL
    lv_init();
    
    // Setup display driver
    // ... (platform-specific code)
    
    // Create the UI
    forma_ui_create();
    
    // Main loop
    while(1) {
        lv_task_handler();
        // ... (platform-specific delay)
    }
    
    return 0;
}
```

## API Reference

### `LVGLRenderer<MaxOutput>`

Template class for generating LVGL C code.

**Template Parameters:**
- `MaxOutput`: Maximum size of generated code buffer (default: 16384 bytes)

**Methods:**

#### `void generate(const Document& doc)`
Generate C code from a parsed Forma document.

**Parameters:**
- `doc`: The Forma document (IR) to convert

**Example:**
```cpp
LVGLRenderer<8192> renderer;
renderer.generate(document);
```

#### `std::string_view get_output() const`
Get the generated code as a string view.

**Returns:** View of the generated C code

#### `const char* c_str() const`
Get the generated code as a C string.

**Returns:** Null-terminated C string

## Limitations

### Current Limitations

1. **Flat Hierarchy**: All widgets are created at root level (children support coming)
2. **Static Layout**: No responsive/dynamic layouts yet
3. **Limited Animation Properties**: Only x, y, width, height, and opacity are supported
4. **Basic Properties**: Advanced LVGL properties require manual coding

### Planned Features

- [ ] Hierarchical widget trees (parent-child relationships)
- [ ] More animatable properties (colors, rotations, etc.)
- [ ] Animation callbacks and chaining
- [ ] Keyframe animations
- [ ] Style generation
- [ ] Theme support
- [ ] Custom widget registration
- [ ] Reactive property bindings
- [x] Layout containers (flex, grid)

## Testing

Run the test suite:

```bash
g++ -std=c++20 -I. lvgl_demo.cpp -o lvgl_demo
./lvgl_demo
```

Or compile and run the full compiler example:

```bash
g++ -std=c++20 -I. forma_to_lvgl.cpp -o forma_to_lvgl
./forma_to_lvgl
```

## Performance

- **Compile Time**: Entire code generation is `constexpr`-capable
- **Generated Code**: Zero overhead - produces idiomatic LVGL calls
- **Binary Size**: No additional runtime overhead beyond LVGL itself
- **Memory**: Stack-based generation, no dynamic allocation

## Contributing

Areas for contribution:
- Additional widget mappings
- More property support
- Event handler generation
- Animation support

## License

Part of the Forma language project.
