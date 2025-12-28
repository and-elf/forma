# CMake Auto-Download Feature

The forma CMake generator plugin now automatically downloads and installs CMake if it's not found on your system.

## How it Works

When the CMake generator is initialized:

1. **Check System CMake**: First checks if `cmake` is available on your PATH
2. **Check Downloaded CMake**: If not on PATH, checks for a previously downloaded copy in `~/.forma/tools/cmake`
3. **Auto-Download**: If neither exists, automatically downloads the latest CMake from GitHub releases
4. **Extract and Use**: Extracts CMake to `~/.forma/tools/cmake` and uses it for builds

## Platform Support

- **Linux**: Downloads `cmake-3.28.1-linux-x86_64.tar.gz`
- **macOS**: Downloads `cmake-3.28.1-macos-universal.tar.gz`
- **Windows**: Downloads `cmake-3.28.1-windows-x86_64.zip`

## Requirements

For downloading to work, you need either:
- `curl` (preferred)
- `wget` (fallback)

## Usage

```cpp
#include "cmake_generator.hpp"

CMakeGenerator generator;
BuildContext ctx;

// CMake will be auto-downloaded if needed
generator.init(&ctx);

// Configure project
generator.set_config(config);
generator.generate_cmakelists("build/CMakeLists.txt");

// Run cmake configure (uses downloaded cmake if needed)
generator.run_cmake_configure();

// Build project
generator.run_cmake_build();
```

## Manual Usage

You can also use the downloader directly:

```cpp
#include "cmake_downloader.hpp"

using namespace forma::cmake;

// Check if cmake exists
if (!CMakeDownloader::is_cmake_available()) {
    // Get cmake path (downloads if needed)
    std::string cmake_path = CMakeDownloader::ensure_cmake_available();
    
    if (!cmake_path.empty()) {
        // Use cmake_path to run cmake
        std::string cmd = cmake_path + " --version";
        system(cmd.c_str());
    }
}
```

## Installation Location

Downloaded CMake is installed to:
- Linux/macOS: `~/.forma/tools/cmake/`
- Windows: `%USERPROFILE%\.forma\tools\cmake\`

The download is approximately 50MB and happens only once per system.

## Benefits

- **Zero Configuration**: No need to install CMake manually
- **Consistent Versions**: All developers use the same CMake version
- **CI/CD Friendly**: Works in containerized environments without pre-installed tools
- **Offline After First Download**: Once downloaded, works without internet connection

## Troubleshooting

### Download Fails

If the automatic download fails:
1. Check internet connectivity
2. Ensure `curl` or `wget` is installed
3. Check disk space in home directory
4. Install CMake manually and add to PATH

### Permission Issues

On Linux/macOS, ensure `~/.forma/tools` is writable:
```bash
mkdir -p ~/.forma/tools
chmod 755 ~/.forma/tools
```

## Version Updates

To update the CMake version, edit `cmake_downloader.hpp` and change the version number in `get_download_url()`.
