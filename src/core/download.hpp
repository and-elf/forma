#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include <optional>

namespace forma::download {

// Download progress callback
// Parameters: bytes_downloaded, total_bytes (0 if unknown)
using ProgressCallback = std::function<void(size_t, size_t)>;

// Download options
struct DownloadOptions {
    // Maximum number of redirects to follow
    int max_redirects = 10;
    
    // Connection timeout in seconds
    int timeout_seconds = 30;
    
    // Follow redirects
    bool follow_redirects = true;
    
    // Verify SSL certificates
    bool verify_ssl = true;
    
    // User agent string
    std::string user_agent = "Forma/0.1.0";
    
    // Progress callback (optional)
    ProgressCallback progress_callback;
};

// Download result
struct DownloadResult {
    bool success = false;
    int http_code = 0;
    std::string error_message;
    size_t bytes_downloaded = 0;
};

/**
 * Download a file from a URL to a local path
 * 
 * @param url Source URL (http:// or https://)
 * @param output_path Destination file path
 * @param options Download options
 * @return Download result with success status and details
 */
DownloadResult download_file(
    const std::string& url,
    const std::string& output_path,
    const DownloadOptions& options = DownloadOptions{}
);

/**
 * Download content to memory
 * 
 * @param url Source URL
 * @param options Download options
 * @return Downloaded content or empty optional on failure
 */
std::optional<std::string> download_to_memory(
    const std::string& url,
    const DownloadOptions& options = DownloadOptions{}
);

/**
 * Extract an archive to a directory
 * Supports: .tar.gz, .tar.bz2, .tar.xz, .zip, .7z
 * 
 * @param archive_path Path to archive file
 * @param output_dir Directory to extract to (created if doesn't exist)
 * @param strip_components Remove N leading path components (like tar --strip-components)
 * @return True on success, false on error
 */
bool extract_archive(
    const std::string& archive_path,
    const std::string& output_dir,
    int strip_components = 0
);

/**
 * Download and extract an archive in one operation
 * Archive file is automatically deleted after extraction
 * 
 * @param url Archive URL
 * @param output_dir Extraction directory
 * @param strip_components Path components to strip
 * @param options Download options
 * @return True on success
 */
bool download_and_extract(
    const std::string& url,
    const std::string& output_dir,
    int strip_components = 0,
    const DownloadOptions& options = DownloadOptions{}
);

} // namespace forma::download
