# Forma Init Command

The `forma init` command initializes a new Forma project with the necessary directory structure and configuration files.

## Usage

```bash
forma init [options]
```

## Options

| Flag | Description | Default |
|------|-------------|---------|
| `--name <name>` | Project name | `myapp` |
| `--project <path>` | Project directory | `.` (current directory) |
| `--build <system>` | Build system | `cmake` |
| `--target <triple>` | Target architecture for cross-compilation | _(none)_ |
| `--renderer <backend>` | Rendering backend | `lvgl` |
| `-v, --verbose` | Enable verbose output | _disabled_ |

## Examples

### Basic Project

Create a new project with default settings in the current directory:

```bash
forma init
```

This creates:
- `forma.toml` - Project configuration
- `src/main.forma` - Main application file
- `lib/` - Library directory for imports
- `build/` - Build output directory
- `assets/` - Asset files directory
- `.gitignore` - Git ignore file
- `README.md` - Project documentation

### Named Project in New Directory

Create a project with a specific name in a new directory:

```bash
forma init --name my-awesome-app --project my-awesome-app
```

### Cross-Compilation Project

Create a project configured for ARM64 cross-compilation:

```bash
forma init \
  --name embedded-app \
  --project embedded-app \
  --target aarch64-linux-gnu \
  --verbose
```

Supported cross-compilation targets:
- `aarch64-linux-gnu` - ARM64/AArch64 Linux
- `arm-linux-gnueabihf` - ARM 32-bit Linux (hard-float)
- `x86_64-w64-mingw32` - Windows x64
- `riscv64-linux-gnu` - RISC-V 64-bit Linux

### Custom Build System and Renderer

```bash
forma init \
  --name gui-app \
  --project gui-app \
  --build cmake \
  --renderer lvgl
```

## Generated Project Structure

After running `forma init`, your project will have this structure:

```
my-project/
├── forma.toml          # Project configuration
├── README.md           # Project documentation
├── .gitignore          # Git ignore patterns
├── src/
│   └── main.forma      # Main application entry point
├── lib/
│   └── forma/          # Library search path
├── build/              # Build outputs (gitignored)
└── assets/             # Asset files (images, fonts, etc.)
```

## Generated Files

### forma.toml

The project configuration file contains:
- Package metadata (name, version, authors)
- Import paths for modules
- Build system configuration
- Target architecture (if cross-compiling)
- Renderer backend settings
- LSP settings
- Language feature flags

Example:

```toml
[package]
name = "myapp"
version = "0.1.0"

[build]
system = "cmake"
standard = "c++20"
binary-name = "myapp"

# Cross-compilation (optional)
target = "aarch64-linux-gnu"

[renderer]
backend = "lvgl"
```

### src/main.forma

A basic "Hello World" application using the specified renderer:

```forma
// myapp - Main Application

import forma.widgets
import forma.layout

App {
    title: "myapp"
    width: 800
    height: 600

    Column {
        alignment: "center"
        spacing: 20

        Text {
            content: "Hello, Forma!"
            size: 24
            bold: true
        }

        Button {
            text: "Click Me"
            width: 200
            height: 50

            onClick: {
                // Handle button click
                print("Button clicked!")
            }
        }
    }
}
```

### README.md

Basic project documentation with:
- Building instructions
- Configuration notes
- Cross-compilation info (if applicable)

## Auto-Download Features

When initializing a project with cross-compilation:

1. **CMake**: Checks if CMake is available, informs user it will auto-download if needed
2. **Toolchain**: Checks for cross-compiler, informs user it will auto-download on first build

Example output:

```
Initializing Forma project: embedded-app

Configuration:
  Name: embedded-app
  Build system: cmake
  Renderer: lvgl
  Target: aarch64-linux-gnu
  Directory: embedded-app

✓ Created directory structure
✓ Created forma.toml
✓ Created src/main.forma
✓ Created .gitignore
✓ Created README.md
Checking for CMake...
✓ CMake found on system PATH
Checking for aarch64-linux-gnu toolchain...
Toolchain for aarch64-linux-gnu not found.
It will be downloaded automatically when building.
Supported targets: aarch64-linux-gnu, arm-linux-gnueabihf, x86_64-w64-mingw32, riscv64-linux-gnu

✓ Project initialized successfully!

Next steps:
  cd embedded-app
  forma src/main.forma

For more information, see README.md
```

## Next Steps After Init

After initializing your project:

1. **Navigate to project directory**:
   ```bash
   cd my-project
   ```

2. **Edit source code**:
   ```bash
   vim src/main.forma
   ```

3. **Build and run**:
   ```bash
   forma src/main.forma
   ```

## Integration with Build System

The generated `forma.toml` is used by:
- Forma compiler to locate imports and configure output
- LSP server for language features
- Build plugins (CMake generator) for native compilation

When you build the project, if cross-compilation is configured:
1. CMake will be downloaded to `~/.forma/tools/cmake` if not on PATH
2. Toolchain will be downloaded to `~/.forma/toolchains/<target>/` if not on system
3. Build system automatically uses the correct compilers

## Customization

After initialization, you can customize:

1. **forma.toml**: Adjust settings, add dependencies, change build options
2. **src/main.forma**: Implement your application logic
3. **Directory structure**: Add more source files, libraries, assets

## See Also

- [forma.toml Configuration](../README.md#configuration)
- [CMake Auto-Download](../plugins/cmake-generator/CMAKE_AUTO_DOWNLOAD.md)
- [Toolchain Auto-Download](../plugins/cmake-generator/TOOLCHAIN_AUTO_DOWNLOAD.md)
- [Build Systems](../README.md#build-systems)
