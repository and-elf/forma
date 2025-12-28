# Forma Build Options

## FORMA_ENABLE_DOWNLOADS

**Default:** OFF  
**Dependencies:** libcurl, OpenSSL, zlib  

Enables HTTP/HTTPS download functionality via the `forma_download` library. When enabled:
- Fetches libcurl 8.5.0 from source via CPM
- Builds with HTTP/HTTPS support only (no FTP, SMTP, etc.)
- Links the forma executable with libcurl
- Adds `FORMA_HAS_DOWNLOAD` preprocessor definition

### Usage

```bash
# Build without download support (default)
cmake ..
make

# Build with download support
cmake -DFORMA_ENABLE_DOWNLOADS=ON ..
make
```

### When to Enable

Enable this option if you need:
- The cmake-generator plugin (downloads toolchains)
- Any plugin that needs to fetch remote resources
- The download utility API in your plugin

### When to Disable

Keep this disabled (default) for:
- LSP server builds (no network needed)
- Static builds without external dependencies
- Minimal builds for embedded systems
- CI/CD systems without internet access

## Build Size Impact

| Configuration | Dependencies | Build Time |
|--------------|--------------|------------|
| Downloads OFF | None | ~5s |
| Downloads ON | libcurl, OpenSSL, zlib | ~30s |

