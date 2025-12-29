# Forma ESP32 LVGL Renderer Plugin

Generates ESP-IDF compatible C code with LVGL for ESP32 microcontrollers. This plugin allows you to write your UI in Forma and deploy it to ESP32 devices.

## Features

- **ESP32 Target**: Generates code for ESP-IDF framework
- **LVGL Integration**: Uses LVGL graphics library for UI rendering
- **FreeRTOS**: Properly handles LVGL tasks in FreeRTOS environment
- **Component Ready**: Generates files ready for ESP-IDF project integration

## Prerequisites

- **ESP-IDF**: Install ESP-IDF (v5.0+)
- **LVGL**: Add LVGL component to your ESP-IDF project
- **Display Driver**: Configure your display (SPI, I2C, etc.)

## Installation

### 1. Install ESP-IDF

```bash
# Follow official ESP-IDF installation guide
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
```

### 2. Add LVGL Component

Add LVGL to your ESP-IDF project's `components/` directory or use managed components:

```bash
cd your-esp32-project
idf.py add-dependency "lvgl/lvgl^8.3.0"
```

## Usage

### 1. Write Your Forma UI

```forma
Window {
    title: "ESP32 App"
    
    Label {
        id: statusLabel
        text: "Hello ESP32!"
        x: 10
        y: 10
    }
    
    Button {
        id: mainButton
        text: "Toggle LED"
        x: 50
        y: 50
        width: 120
        height: 50
    }
    
    Slider {
        id: brightnessSlider
        x: 50
        y: 120
        width: 200
        min: 0
        max: 100
        value: 50
    }
}
```

### 2. Compile with ESP32 LVGL Plugin

```bash
forma compile myapp.fml --plugin esp32-lvgl --output esp32-build/
```

### 3. Integrate into ESP-IDF Project

Copy generated files to your ESP-IDF project:

```bash
cp esp32-build/forma_ui.h your-esp32-project/main/
cp esp32-build/forma_ui.c your-esp32-project/main/
```

### 4. Update main.c

```c
#include "forma_ui.h"

void app_main(void) {
    // Create LVGL UI task
    xTaskCreate(forma_ui_task, "lvgl_task", 8192, NULL, 5, NULL);
}
```

### 5. Configure Display Driver

Run `idf.py menuconfig` and configure:
- LVGL settings
- Display driver (SPI TFT, I2C OLED, etc.)
- Touch driver (if applicable)

### 6. Build and Flash

```bash
idf.py build
idf.py flash monitor
```

## Project Structure

Generated files create this structure:

```
esp32-build/
├── forma_ui.h          # Header with function declarations
└── forma_ui.c          # Implementation with LVGL code
```

Integrate into ESP-IDF project:

```
your-esp32-project/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── forma_ui.h      # ← Copy here
│   └── forma_ui.c      # ← Copy here
└── components/
    └── lvgl/           # ← Add LVGL component
```

## Supported Widgets

- **Label**: Text display
- **Button**: Clickable buttons
- **Slider**: Value selection
- **Switch**: On/off toggle
- **Arc**: Circular progress/input
- **Roller**: Scrollable selection

## Display Drivers

The plugin generates generic LVGL code. Configure your specific display driver in ESP-IDF:

### SPI TFT (ST7789, ILI9341)
```bash
idf.py add-dependency "espressif/esp_lvgl_port^1.0.0"
```

### I2C OLED (SSD1306)
```bash
# Add SSD1306 driver component
```

### RGB LCD
```bash
# Configure RGB interface in menuconfig
```

## Configuration

Edit `sdkconfig` or use `idf.py menuconfig`:

```
Component config → LVGL configuration
  - Buffer size
  - Color depth
  - Display resolution
  - Task priority
```

## Memory Considerations

ESP32 has limited RAM. Optimize LVGL settings:

- Reduce buffer size for small displays
- Use 16-bit color depth instead of 32-bit
- Disable unused LVGL features
- Use PSRAM if available

## Performance Tips

1. **Use Hardware Acceleration**: Enable SPI DMA for displays
2. **Optimize Refresh Rate**: Balance between smoothness and CPU usage
3. **Task Priority**: Adjust LVGL task priority based on your needs
4. **Double Buffering**: Enable for smoother animations (requires more RAM)

## Debugging

Enable ESP32 logging:

```c
esp_log_level_set("FormaUI", ESP_LOG_DEBUG);
esp_log_level_set("lvgl", ESP_LOG_DEBUG);
```

Monitor output:
```bash
idf.py monitor
```

## Example Projects

See `examples/esp32-demo/` for complete ESP-IDF project examples:

- Basic button and label
- Temperature monitor with gauge
- Multi-screen navigation
- Touch input handling

## Hardware Requirements

**Minimum**:
- ESP32 (any variant)
- Display (SPI/I2C)
- 4MB Flash

**Recommended**:
- ESP32-S3 or ESP32-C3
- SPI TFT display with touch
- 8MB Flash + 2MB PSRAM

## Troubleshooting

**Display not working?**
- Check SPI/I2C pin configuration
- Verify display initialization in driver
- Check power supply to display

**Out of memory?**
- Reduce LVGL buffer size
- Enable PSRAM
- Simplify UI complexity

**Slow rendering?**
- Enable SPI DMA
- Reduce color depth
- Optimize refresh rate

## Building

```bash
mkdir build && cd build
cmake ..
make forma_esp32_lvgl
```

## License

MIT License - See main Forma repository for details.
