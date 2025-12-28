# Forma Download Utility

The `forma::download` library provides HTTP/HTTPS download functionality for the Forma compiler and plugins.

## Build Configuration

⚠️ **The download utility is optional and must be enabled at build time:**

```bash
cmake -DFORMA_ENABLE_DOWNLOADS=ON ..
```

By default, this option is **OFF** to keep builds lightweight for components that don't need download capabilities (like LSP servers). When enabled, forma will be linked with libcurl.

## Features

- ✅ HTTP/HTTPS file downloads via libcurl
- ✅ Progress callbacks for download tracking  
- ✅ Download to file or memory
- ✅ SSL certificate verification
- ✅ Redirect following
- ⏳ Archive extraction (tar.gz, zip) - coming soon with libarchive

## Usage for Plugin Developers

Include the download header in your plugin:

```cpp
#include "core/download.hpp"

using namespace forma::download;
```

### Download a File

```cpp
// Simple download
auto result = download_file(
    "https://github.com/user/repo/releases/download/v1.0/file.tar.gz",
    "/tmp/downloaded_file.tar.gz"
);

if (result.success) {
    std::cout << "Downloaded " << result.bytes_downloaded << " bytes\n";
} else {
    std::cerr << "Error: " << result.error_message << "\n";
}
```

### Download with Progress Callback

```cpp
DownloadOptions opts;
opts.progress_callback = [](size_t current, size_t total) {
    if (total > 0) {
        int percent = (current * 100) / total;
        std::cout << "\rProgress: " << percent << "%" << std::flush;
    }
};

auto result = download_file(url, output_path, opts);
```

### Download to Memory

```cpp
auto content = download_to_memory("https://api.example.com/data.json");
if (content) {
    // Process content
    parse_json(*content);
}
```

## Building Plugins with Download Support

When building your plugin, make sure download support is enabled in your build:

```bash
cmake -DFORMA_ENABLE_DOWNLOADS=ON ..
```

Then link against `forma_download`:

```cmake
# Only link if download support is enabled
if(FORMA_ENABLE_DOWNLOADS)
    add_library(my_plugin SHARED plugin.cpp)
    target_link_libraries(my_plugin 
        PRIVATE 
            forma_download
    )
    target_compile_definitions(my_plugin PRIVATE FORMA_HAS_DOWNLOAD)
endif()
```

In your plugin code, you can check if download support was compiled in:

```cpp
#ifdef FORMA_HAS_DOWNLOAD
    #include "core/download.hpp"
    using namespace forma;
    
    // Use download functionality
    auto result = download_file("https://...", "/tmp/file");
#else
    // Provide alternative or error message
    std::cerr << "This build was not compiled with download support\n";
#endif
```

## Future: Archive Extraction

Once libarchive is integrated, you'll be able to:

```cpp
// Download and extract in one step
download_and_extract(
    "https://example.com/toolchain.tar.gz",
    "/opt/toolchain",
    1  // strip 1 leading path component
);

// Or extract separately
extract_archive("file.tar.gz", "/output/dir");
```

## Use Cases

- **cmake-generator**: Download cross-compilation toolchains
- **package-manager**: Fetch dependencies from URLs
- **asset-downloader**: Pull external assets for bundling
- **update-checker**: Fetch release information

## Options Reference

```cpp
struct DownloadOptions {
    int max_redirects = 10;        // Follow up to 10 redirects
    int timeout_seconds = 30;      // 30 second timeout
    bool follow_redirects = true;  // Follow HTTP redirects
    bool verify_ssl = true;        // Verify SSL certificates
    std::string user_agent = "Forma/0.1.0";
    ProgressCallback progress_callback;  // Optional progress tracking
};
```
