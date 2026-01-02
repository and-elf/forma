#include "download.hpp"
#include <archive.hpp>
#include <curl/curl.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <ctime>

namespace forma::download {

// RAII wrapper for curl handle
class CurlHandle {
    CURL* curl = nullptr;
public:
    CurlHandle() : curl(curl_easy_init()) {}
    ~CurlHandle() { if (curl) curl_easy_cleanup(curl); }
    operator CURL*() { return curl; }
    bool valid() const { return curl != nullptr; }
};

// Callback for writing downloaded data to file
static size_t write_to_file(void* ptr, size_t size, size_t nmemb, void* stream) {
    auto* out = static_cast<std::ofstream*>(stream);
    out->write(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

// Callback for writing to memory
static size_t write_to_memory(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* str = static_cast<std::string*>(userdata);
    str->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

// Callback for writing to a generic writer (IWriteStream)
static size_t write_to_stream(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* wptr = static_cast<forma::io::IWriteStream*>(userdata);
    if (!wptr) return 0;
    size_t bytes = size * nmemb;
    wptr->write(ptr, bytes);
    return bytes;
}

// Progress callback wrapper
struct ProgressData {
    ProgressCallback callback;
};

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, 
                             curl_off_t ultotal, curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;
    
    if (clientp) {
        auto* data = static_cast<ProgressData*>(clientp);
        if (data->callback) {
            data->callback(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
        }
    }
    return 0;
}

DownloadResult download_file(const std::string& url, const std::string& output_path, 
                             const DownloadOptions& options) {
    DownloadResult result;
    
    // Initialize curl
    CurlHandle curl;
    if (!curl.valid()) {
        result.error_message = "Failed to initialize curl";
        return result;
    }
    
    // Open output file
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        result.error_message = "Failed to open output file: " + output_path;
        return result;
    }
    
    // Setup curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, options.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(options.max_redirects));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(options.timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, options.user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, options.verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, options.verify_ssl ? 2L : 0L);
    
    // Setup progress callback if provided
    ProgressData progress_data{options.progress_callback};
    if (options.progress_callback) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    
    // Perform download
    CURLcode res = curl_easy_perform(curl);
    output.close();
    
    if (res != CURLE_OK) {
        result.error_message = curl_easy_strerror(res);
        forma::fs::RealFileSystem realfs;
        if (realfs.exists(output_path)) std::remove(output_path.c_str());
        return result;
    }
    
    // Get response code
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    result.http_code = static_cast<int>(response_code);
    
    // Get downloaded size
    curl_off_t downloaded = 0;
    curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &downloaded);
    result.bytes_downloaded = static_cast<size_t>(downloaded);
    
    // Check for HTTP errors
    if (response_code >= 400) {
        result.error_message = "HTTP error " + std::to_string(response_code);
        forma::fs::RealFileSystem realfs;
        if (realfs.exists(output_path)) std::remove(output_path.c_str());
        return result;
    }
    
    result.success = true;
    return result;
}

std::optional<std::string> download_to_memory(const std::string& url, const DownloadOptions& options) {
    CurlHandle curl;
    if (!curl.valid()) {
        return std::nullopt;
    }
    
    std::string content;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, options.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(options.max_redirects));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(options.timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, options.user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, options.verify_ssl ? 1L : 0L);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return std::nullopt;
    }
    
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code >= 400) {
        return std::nullopt;
    }
    
    return content;
}

bool download_to_stream(const std::string& url, OpenWriteStreamFn writer_fn, const DownloadOptions& options) {
    CurlHandle curl;
    if (!curl.valid()) return false;

    auto writer_ptr = writer_fn(url); // caller may ignore the argument but we need a writer for path
    if (!writer_ptr) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_stream);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, writer_ptr.get());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, options.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(options.max_redirects));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(options.timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, options.user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, options.verify_ssl ? 1L : 0L);

    ProgressData progress_data{options.progress_callback};
    if (options.progress_callback) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) return false;

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code >= 400) return false;

    return true;
}

bool extract_archive(const std::string& archive_path, const std::string& output_dir, 
                    int strip_components) {
    archive::ExtractOptions opts;
    opts.strip_components = strip_components;
    opts.create_dest_dir = true;
    opts.overwrite = true;
    
    auto result = archive::extract_archive(archive_path, output_dir, opts);
    
    if (!result.success) {
        std::cerr << "Archive extraction failed: " << result.error_message << "\n";
        return false;
    }
    
    return true;
}

bool download_and_extract(const std::string& url, const std::string& output_dir,
                         int strip_components, const DownloadOptions& options) {
    // Download to temporary file
    namespace fs = std::filesystem;
    fs::path temp_dir = fs::temp_directory_path();
    fs::path temp_file = temp_dir / ("forma_download_" + std::to_string(std::time(nullptr)));
    
    // Determine file extension from URL
    std::string url_lower = url;
    std::transform(url_lower.begin(), url_lower.end(), url_lower.begin(), ::tolower);
    
    if (url_lower.find(".tar.gz") != std::string::npos || url_lower.find(".tgz") != std::string::npos) {
        temp_file += ".tar.gz";
    } else if (url_lower.find(".tar.bz2") != std::string::npos || url_lower.find(".tbz2") != std::string::npos) {
        temp_file += ".tar.bz2";
    } else if (url_lower.find(".tar.xz") != std::string::npos || url_lower.find(".txz") != std::string::npos) {
        temp_file += ".tar.xz";
    } else if (url_lower.find(".zip") != std::string::npos) {
        temp_file += ".zip";
    } else if (url_lower.find(".7z") != std::string::npos) {
        temp_file += ".7z";
    } else {
        temp_file += ".archive";
    }
    
    // Download the archive
    std::cout << "Downloading archive from " << url << "...\n";
    auto result = download_file(url, temp_file.string(), options);
    
    if (!result.success) {
        std::cerr << "Download failed: " << result.error_message << "\n";
        return false;
    }
    
    std::cout << "Download complete, extracting...\n";
    
    // Extract the archive
    bool extract_ok = extract_archive(temp_file.string(), output_dir, strip_components);
    
    // Clean up temp file
    try {
        if (std::filesystem::exists(temp_file)) std::filesystem::remove(temp_file);
    } catch (...) {
        std::cerr << "Warning: Failed to remove temporary file: " << temp_file << "\n";
    }
    
    return extract_ok;
}

} // namespace forma::download

// Host-aware C exported wrappers
#include "../../src/core/host_context.hpp"
#include "../../src/core/fs/fs_copy.hpp"

extern "C" {

bool forma_download_host(void* host_ptr, const char* url_c, const char* output_path_c, const void* /*options_ptr*/) {
    if (!host_ptr || !url_c || !output_path_c) return false;
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!host || !host->filesystem) return false;
    forma::download::DownloadOptions opts;
    return forma_download_to_stream_host(host_ptr, std::string(url_c), std::string(output_path_c), opts);
}

bool forma_download_to_stream_host(void* host_ptr, const std::string& url, const std::string& output_path, const DownloadOptions& options) {
    if (!host_ptr) return false;
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!host || !host->filesystem) return false;

    // Create a writer closure that ignores the incoming path and uses output_path
    auto writer_fn = [host, output_path](const std::string& /*p*/) -> forma::io::WriteStreamPtr {
        return host->stream_io.open_write_stream(output_path);
    };

    return download_to_stream(url, writer_fn, options);
}

bool forma_extract_host(void* host_ptr, const char* archive_path_c, const char* output_dir_c, int strip_components) {
    if (!host_ptr || !archive_path_c || !output_dir_c) return false;
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!host || !host->filesystem) return false;

    namespace fs = std::filesystem;
    fs::path tmpdir = fs::temp_directory_path() / ("forma_extract_" + std::to_string(std::time(nullptr)));
    try { fs::create_directories(tmpdir); } catch(...) { return false; }

    bool ok = forma::download::extract_archive(std::string(archive_path_c), tmpdir.string(), strip_components);
    if (!ok) {
        try { fs::remove_all(tmpdir); } catch(...) {}
        return false;
    }

    bool copied = forma::fs::copy_disk_to_fs(tmpdir.string(), *host->filesystem, std::string(output_dir_c));

    try { fs::remove_all(tmpdir); } catch(...) {}
    return copied;
}

bool forma_download_and_extract_host(void* host_ptr, const char* url_c, const char* output_dir_c, int strip_components, const void* /*opts*/) {
    if (!host_ptr || !url_c || !output_dir_c) return false;
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!host || !host->filesystem) return false;
    forma::download::DownloadOptions opts;
    // Stream download into a temporary in-memory buffer? Instead, download to a temporary
    // memory-backed path inside host filesystem and then call memory-extract using in-memory bytes.
    // We'll stream into a host filesystem path and then read it back into memory for extraction.
    std::string tmp_path = std::string(output_dir_c);
    if (!tmp_path.empty() && tmp_path.back() != '/') tmp_path.push_back('/');
    tmp_path += "__download_tmp_archive";
    // Ensure parent dir exists
    try {
        auto parent = std::filesystem::path(tmp_path).parent_path().string();
        if (!parent.empty()) host->filesystem->create_dirs(parent);
    } catch(...) {}

    bool ok = forma_download_to_stream_host(host_ptr, std::string(url_c), tmp_path, opts);
    if (!ok) return false;

    // Read back into memory and extract via memory-extract
    std::string data;
    try {
        data = host->filesystem->read_file(tmp_path);
    } catch(...) { return false; }

    extern bool forma_extract_from_memory_host(void* host_ptr, const char* data, size_t len, const char* dest_dir, int strip_components);
    bool ex_ok = forma_extract_from_memory_host(host_ptr, data.data(), data.size(), output_dir_c, strip_components);

    return ex_ok;
}

} // extern "C"
