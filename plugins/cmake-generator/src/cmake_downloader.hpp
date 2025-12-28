#pragma once
#include <string>
#include <cstdlib>

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
        std::string url = get_download_url();
        if (url.empty()) {
            return false;
        }
        
        // Create install directory
        std::string cmd = "mkdir -p \"" + install_dir + "\"";
        if (system(cmd.c_str()) != 0) {
            return false;
        }
        
        // Download
        std::string filename = install_dir + "/cmake-download.tar.gz";
#if defined(_WIN32)
        filename = install_dir + "/cmake-download.zip";
#endif
        
        cmd = "curl -L -o \"" + filename + "\" \"" + url + "\"";
        if (system(cmd.c_str()) != 0) {
            // Try wget as fallback
            cmd = "wget -O \"" + filename + "\" \"" + url + "\"";
            if (system(cmd.c_str()) != 0) {
                return false;
            }
        }
        
        // Extract
#if defined(_WIN32)
        cmd = "cd \"" + install_dir + "\" && unzip -q cmake-download.zip";
#else
        cmd = "cd \"" + install_dir + "\" && tar xzf cmake-download.tar.gz --strip-components=1";
#endif
        
        if (system(cmd.c_str()) != 0) {
            return false;
        }
        
        // Clean up archive
        cmd = "rm \"" + filename + "\"";
        system(cmd.c_str());
        
        return true;
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
