# Forma ESP32 Build System Plugin

Sets up ESP-IDF environment and toolchain for ESP32 microcontroller development. This plugin handles ESP-IDF installation, project scaffolding, and build system configuration.

## Features

- **Automatic ESP-IDF Setup**: Downloads and installs ESP-IDF (configurable version)
- **Toolchain Management**: Handles ESP32 toolchain installation
- **Project Scaffolding**: Creates proper ESP-IDF project structure
- **Multi-Target Support**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6
- **Version Control**: Pin specific ESP-IDF versions via TOML configuration

## Configuration

Add an `[esp32]` section to your `project.toml`:

```toml
[project]
name = "my-esp32-app"
version = "1.0.0"

[esp32]
idf_version = "v5.1"           # ESP-IDF version to use
target = "esp32s3"             # Target chip
auto_install = true            # Auto-install ESP-IDF if not found
```

## Usage

```bash
# Setup ESP32 project
forma setup-esp32

# Build
source ~/esp/esp-idf/export.sh
idf.py build flash monitor
```

See full documentation in the file for details.
