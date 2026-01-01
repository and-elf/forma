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
        std::filesystem::remove(output_path); // Clean up partial file
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
        std::filesystem::remove(output_path);
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
        fs::remove(temp_file);
    } catch (...) {
        std::cerr << "Warning: Failed to remove temporary file: " << temp_file << "\n";
    }
    
    return extract_ok;
}

} // namespace forma::download
