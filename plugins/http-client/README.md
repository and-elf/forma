# HTTP Client Plugin for Forma

Utility plugin providing HTTP/HTTPS download capabilities using libcurl.

## Features

- HTTP/HTTPS downloads with SSL verification
- Progress callbacks
- Redirect following
- Configurable timeouts
- User agent customization

## Building

```bash
cmake -B build
cmake --build build
sudo cmake --build build --target install
```

## Usage

```cpp
#include <http-client/download.hpp>

using namespace forma::download;

DownloadOptions opts;
opts.timeout_seconds = 60;
opts.progress_callback = [](size_t current, size_t total) {
    std::cout << "Downloaded: " << current << " / " << total << "\n";
};

auto result = download_file(
    "https://example.com/file.tar.gz",
    "/tmp/file.tar.gz",
    opts
);

if (result.success) {
    std::cout << "Downloaded " << result.bytes_downloaded << " bytes\n";
} else {
    std::cerr << "Error: " << result.error_message << "\n";
}
```

## Dependencies

- **libcurl** - Automatically fetched via CPM
- **OpenSSL** - System dependency for HTTPS support

## Integration

```cmake
find_package(FormaHTTPClient REQUIRED)
target_link_libraries(your_target PRIVATE FormaPlugins::forma_http_client)
```
