#pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <filesystem>

#ifdef FORMA_HAS_DOWNLOAD
#include "core/download.hpp"
#endif

namespace forma::cmake {

// Simple utility to download and install CMake if not found
class CMakeDownloader {
public:
    // Check if cmake is available on PATH
    static bool is_cmake_available() {
        return system("cmake --version > /dev/null 2>&1") == 0;
    }
    
    // Get the platform-specific download URL for latest CMake
    static std::string get_download_url() {
#if defined(__linux__)
        return "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-linux-x86_64.tar.gz";
#elif defined(__APPLE__)
        return "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-macos-universal.tar.gz";
#elif defined(_WIN32)
        return "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.zip";
#else
        return "";
#endif
    }
    
    // Download and extract CMake to the specified directory
    static bool download_and_install(const std::string& install_dir) {
#ifdef FORMA_HAS_DOWNLOAD
        std::string url = get_download_url();
        if (url.empty()) {
            std::cerr << "Unsupported platform for CMake download\n";
            return false;
        }
        
        // Create install directory
        std::filesystem::create_directories(install_dir);
        
        // Download with progress
        std::string filename = install_dir + "/cmake-download.tar.gz";
#if defined(_WIN32)
        filename = install_dir + "/cmake-download.zip";
#endif
        
        std::cout << "Downloading CMake from " << url << "...\n";
        
        download::DownloadOptions opts;
        opts.follow_redirects = true;
        opts.max_redirects = 10;
        opts.timeout_seconds = 300; // 5 minutes
        opts.progress_callback = [](size_t current, size_t total) {
            if (total > 0) {
                int percent = (current * 100) / total;
                std::cout << "\rProgress: " << percent << "% (" 
                         << current / 1024 / 1024 << " MB / " 
                         << total / 1024 / 1024 << " MB)" << std::flush;
            }
        };
        
        auto result = download::download_file(url, filename, opts);
        
        if (!result.success) {
            std::cerr << "\nDownload failed: " << result.error_message << "\n";
            return false;
        }
        
        std::cout << "\nDownload complete (" << result.bytes_downloaded / 1024 / 1024 << " MB)\n";
        std::cout << "Extracting...\n";
        
        // Extract using system tar/unzip (TODO: use libarchive when available)
#if defined(_WIN32)
        std::string cmd = "cd \"" + install_dir + "\" && unzip -q cmake-download.zip";
#else
        std::string cmd = "cd \"" + install_dir + "\" && tar xzf cmake-download.tar.gz --strip-components=1";
#endif
        
        if (system(cmd.c_str()) != 0) {
            std::cerr << "Extraction failed\n";
            return false;
        }
        
        // Clean up archive
        std::filesystem::remove(filename);
        
        std::cout << "CMake installed to " << install_dir << "\n";
        return true;
#else
        (void)install_dir; // Unused when download support is disabled
        std::cerr << "Download support not compiled in. Build with -DFORMA_ENABLE_DOWNLOADS=ON\n";
        return false;
#endif
    }
    
    // Get the path to cmake binary in the install directory
    static std::string get_cmake_path(const std::string& install_dir) {
#if defined(_WIN32)
        return install_dir + "/bin/cmake.exe";
#else
        return install_dir + "/bin/cmake";
#endif
    }
    
    // Ensure cmake is available, downloading if necessary
    // Returns the path to cmake executable
    static std::string ensure_cmake_available() {
        // First check if cmake is on PATH
        if (is_cmake_available()) {
            return "cmake"; // Use system cmake
        }
        
        // Check if we have a downloaded copy
        std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
#if defined(_WIN32)
        home = std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : "";
#endif
        
        if (home.empty()) {
            return ""; // Can't determine home directory
        }
        
        std::string cmake_dir = home + "/.forma/tools/cmake";
        std::string cmake_bin = get_cmake_path(cmake_dir);
        
        // Check if downloaded cmake exists
        std::string check_cmd = cmake_bin + " --version > /dev/null 2>&1";
        if (system(check_cmd.c_str()) == 0) {
            return cmake_bin; // Use downloaded cmake
        }
        
        // Need to download
        if (!download_and_install(cmake_dir)) {
            return ""; // Download failed
        }
        
        return cmake_bin;
    }
};

} // namespace forma::cmake
