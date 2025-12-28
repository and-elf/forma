# Toolchain Auto-Download

The CMake generator plugin now automatically downloads cross-compilation toolchains when a target triple is specified.

## Supported Targets

The following target architectures are supported:

- `aarch64-linux-gnu` - ARM64/AArch64 Linux (GCC 13.2.0)
- `arm-linux-gnueabihf` - ARM 32-bit Linux hard-float (GCC 13.2.0)
- `x86_64-w64-mingw32` - Windows x64 cross-compilation (MinGW-w64 GCC 13.2.0)
- `riscv64-linux-gnu` - RISC-V 64-bit Linux (GCC nightly 2023.11.08)
- `x86_64-linux-gnu` - x86_64 Linux (native or cross)

## Usage

### Setting Target Triple

```cpp
CMakeGeneratorConfig config;
config.target_triple = "aarch64-linux-gnu"; // Cross-compile for ARM64

CMakeGenerator generator;
generator.set_config(config);
generator.init(&context);  // Will auto-download toolchain if needed
```

### Build Flow

1. **Initialization**: When `init()` is called with a `target_triple` set:
   - Checks if the cross-compiler is on the system PATH
   - If not found, downloads from official sources
   - Installs to `~/.forma/toolchains/<target>/`
   - Caches compiler path in memory

2. **Configuration**: When `run_cmake_configure()` is called:
   - Automatically sets `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`
   - Derives C++ compiler from C compiler (gcc → g++, clang → clang++)
   - Passes correct flags to CMake

3. **Building**: Regular `run_cmake_build()` uses the configured toolchain

## Installation Locations

Toolchains are downloaded to:
```
~/.forma/toolchains/
├── aarch64-linux-gnu/
│   └── bin/
│       ├── aarch64-none-linux-gnu-gcc
│       ├── aarch64-none-linux-gnu-g++
│       └── ...
├── arm-linux-gnueabihf/
│   └── bin/
│       ├── arm-none-linux-gnueabihf-gcc
│       └── ...
└── x86_64-w64-mingw32/
    └── mingw64/
        └── bin/
            ├── x86_64-w64-mingw32-gcc.exe
            └── ...
```

## Example: Cross-Compile for ARM64

```cpp
// Configure for ARM64 Linux
CMakeGeneratorConfig config;
config.project_name = "MyApp";
config.target_name = "myapp";
config.target_triple = "aarch64-linux-gnu";  // Enable cross-compilation
config.source_files = {"main.cpp", "utils.cpp"};
config.build_type = "Release";

// Initialize (downloads toolchain if needed)
CMakeGenerator gen;
gen.set_config(config);
gen.init(&context);

// Generate CMakeLists.txt
gen.generate_cmakelists("CMakeLists.txt");

// Configure and build with cross-compiler
gen.run_cmake_configure();
gen.run_cmake_build();
```

## How It Works

### Toolchain Detection
1. Check if compiler is already on PATH (e.g., `aarch64-none-linux-gnu-gcc --version`)
2. If found, use system compiler
3. Otherwise, proceed to download

### Download Process
1. Get download URL for target from toolchain database
2. Use `curl` (or `wget` as fallback) to download archive
3. Extract to `~/.forma/toolchains/<target>/`
4. Return path to compiler binary

### CMake Integration
The plugin automatically:
- Sets `-DCMAKE_C_COMPILER=<path>` 
- Sets `-DCMAKE_CXX_COMPILER=<path>` (derived from C compiler)
- Preserves all other CMake options

## Requirements

### Linux Host (for cross-compilation)
- `curl` or `wget` - for downloading
- `tar` - for extracting `.tar.gz` and `.tar.xz` archives
- `7z` or `unzip` - for Windows toolchain (`.7z` or `.zip` format)

### Disk Space
Each toolchain requires approximately:
- ARM toolchains: ~400-600 MB
- MinGW-w64: ~300 MB
- RISC-V: ~500 MB

## Toolchain Sources

All toolchains are downloaded from official sources:
- **ARM toolchains**: ARM Developer (developer.arm.com)
- **MinGW-w64**: niXman's builds on GitHub (recommended by MinGW-w64 project)
- **RISC-V**: Official riscv-collab releases on GitHub

## Troubleshooting

### Download Fails
If download fails, check:
1. Internet connection
2. `curl` or `wget` is installed: `which curl wget`
3. Disk space in `~/.forma/toolchains/`

The plugin will show diagnostic messages:
```
Info: Checking for aarch64-linux-gnu toolchain...
Info: Downloaded toolchain to: /home/user/.forma/toolchains/aarch64-linux-gnu/bin/aarch64-none-linux-gnu-gcc
```

### Extraction Fails
- For `.tar.xz`: Ensure `tar` with xz support is available
- For `.7z`: Install `p7zip` or `7zip`
  ```bash
  # Ubuntu/Debian
  sudo apt-get install p7zip-full
  
  # Arch
  sudo pacman -S p7zip
  ```

### Compiler Not Found After Download
Check that the toolchain extracted correctly:
```bash
ls -la ~/.forma/toolchains/<target>/bin/
```

Should show compiler binaries (gcc, g++, etc.)

### Using System Toolchain
To use a system-installed cross-compiler instead of auto-downloading:
1. Install the toolchain package
2. Ensure compiler is on PATH
3. Plugin will detect and use it automatically

Example for Debian/Ubuntu:
```bash
# Install ARM64 cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Now the plugin will use system compiler
```

### Unsupported Target
If you get: "Failed to find or download toolchain for <target>", the target is not in the database.

Supported targets list is shown in the diagnostic:
```
Error: Failed to find or download toolchain for custom-target
Info: Supported targets: aarch64-linux-gnu, arm-linux-gnueabihf, x86_64-w64-mingw32, riscv64-linux-gnu
```

### Manual Cleanup
To remove downloaded toolchains:
```bash
rm -rf ~/.forma/toolchains/
```

They will be re-downloaded on next use if needed.

## Adding New Targets

To add support for a new target, edit [toolchain_downloader.hpp](src/toolchain_downloader.hpp) and add an entry to `get_toolchain_database()`:

```cpp
db["<target-triple>"] = {
    "Toolchain Name",
    "https://download-url.com/toolchain.tar.xz",
    "archive-filename.tar.xz",
    "bin",  // Relative path to bin directory after extraction
    "prefix-gcc"  // Compiler executable name
};
```

## Performance

- **First use**: Downloads toolchain (~300-600 MB per target)
- **Subsequent uses**: Uses cached toolchain (instant)
- **Build time**: No overhead vs manual toolchain setup

## Security Notes

- Downloads are from official sources (ARM Developer, GitHub releases)
- No checksum verification currently implemented
- Downloads over HTTPS when available
- Toolchains stored in user directory (no root required)

Consider verifying checksums manually for production use.
