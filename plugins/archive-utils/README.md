# Archive Utils Plugin for Forma

Utility plugin providing archive extraction capabilities using libarchive.

## Features

- Extract tar.gz, tar.bz2, tar.xz, zip, and 7z archives
- Auto-detect archive format
- Strip leading path components
- Progress callbacks
- Overwrite control

## Building

```bash
cmake -B build
cmake --build build
sudo cmake --build build --target install
```

## Usage

```cpp
#include <archive-utils/archive.hpp>

using namespace forma::archive;

ExtractOptions opts;
opts.strip_components = 1;  // Remove top-level directory
opts.progress_callback = [](size_t current, size_t total) {
    std::cout << "Extracted: " << current << " / " << total << " files\n";
};

auto result = extract_archive(
    "/tmp/archive.tar.gz",
    "/opt/destination",
    opts
);

if (result.success) {
    std::cout << "Extracted " << result.files_extracted << " files\n";
    std::cout << "Total: " << result.bytes_extracted << " bytes\n";
} else {
    std::cerr << "Error: " << result.error_message << "\n";
}
```

## Supported Formats

- `.tar.gz`, `.tgz` - Gzip compressed tar
- `.tar.bz2`, `.tbz2` - Bzip2 compressed tar
- `.tar.xz`, `.txz` - XZ compressed tar
- `.tar` - Uncompressed tar
- `.zip` - ZIP archives
- `.7z` - 7-Zip archives

## Dependencies

- **libarchive** - Automatically fetched via CPM

## Integration

```cmake
find_package(FormaArchiveUtils REQUIRED)
target_link_libraries(your_target PRIVATE FormaPlugins::forma_archive_utils)
```
