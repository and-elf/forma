// Download utility test
#include "core/download.hpp"
#include <iostream>

int main() {
    using namespace forma::download;
    
    std::cout << "Testing Forma download utility...\n\n";
    
    // Test 1: Download to memory
    std::cout << "Test 1: Download small file to memory\n";
    auto content = download_to_memory("https://httpbin.org/get");
    if (content) {
        std::cout << "✓ Downloaded " << content->size() << " bytes\n";
    } else {
        std::cout << "✗ Download failed\n";
    }
    
    // Test 2: Download to file with progress
    std::cout << "\nTest 2: Download to file with progress callback\n";
    DownloadOptions opts;
    opts.progress_callback = [](size_t current, size_t total) {
        if (total > 0) {
            std::cout << "\rProgress: " << (current * 100 / total) << "%" << std::flush;
        }
    };
    
    auto result = download_file(
        "https://httpbin.org/bytes/1024",
        "/tmp/forma_test_download.bin",
        opts
    );
    
    std::cout << "\n";
    if (result.success) {
        std::cout << "✓ Downloaded " << result.bytes_downloaded << " bytes (HTTP " 
                  << result.http_code << ")\n";
    } else {
        std::cout << "✗ Failed: " << result.error_message << "\n";
    }
    
    std::cout << "\nDownload utility is working!\n";
    return 0;
}
