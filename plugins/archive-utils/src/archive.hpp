#pragma once

#include <string>
#include <functional>

namespace forma::archive {

// Options for archive extraction
struct ExtractOptions {
    // Number of leading path components to strip (like tar --strip-components)
    int strip_components = 0;
    
    // Optional callback for extraction progress
    // Parameters: current_file, total_files
    std::function<void(size_t, size_t)> progress_callback;
    
    // Overwrite existing files
    bool overwrite = true;
    
    // Create destination directory if it doesn't exist
    bool create_dest_dir = true;
};

// Result of an extraction operation
struct ExtractResult {
    bool success = false;
    std::string error_message;
    size_t files_extracted = 0;
    size_t bytes_extracted = 0;
};

// Supported archive formats (auto-detected by libarchive)
enum class ArchiveFormat {
    Auto,      // Auto-detect from file extension and content
    TarGz,     // .tar.gz, .tgz
    TarBz2,    // .tar.bz2, .tbz2
    TarXz,     // .tar.xz, .txz
    Zip,       // .zip
    Tar,       // .tar (uncompressed)
    SevenZip,  // .7z
};

/**
 * Extract an archive file to a destination directory
 * 
 * @param archive_path Path to the archive file
 * @param dest_dir Destination directory for extracted files
 * @param options Extraction options
 * @return ExtractResult with success status and details
 */
ExtractResult extract_archive(
    const std::string& archive_path,
    const std::string& dest_dir,
    const ExtractOptions& options = ExtractOptions()
);

// Host-aware extract: extracts into a disk tempdir then copies into host filesystem
bool forma_extract_host(void* host_ptr,
    const std::string& archive_path,
    const std::string& dest_dir,
    const ExtractOptions& options = ExtractOptions()
);

// Extract archive from memory buffer into host filesystem
bool forma_extract_from_memory_host(void* host_ptr, const char* data, size_t len, const std::string& dest_dir, int strip_components = 0);

/**
 * List contents of an archive without extracting
 * 
 * @param archive_path Path to the archive file
 * @return Vector of file paths in the archive
 */
std::vector<std::string> list_archive(const std::string& archive_path);

/**
 * Get the detected format of an archive
 * 
 * @param archive_path Path to the archive file
 * @return Detected archive format
 */
ArchiveFormat detect_format(const std::string& archive_path);

} // namespace forma::archive
