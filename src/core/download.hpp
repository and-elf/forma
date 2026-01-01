#pragma once

#include <string>
#include <filesystem>
#include <cstdlib>
#include <iostream>

namespace forma::download {

// Download a file from a URL to a local path
inline bool download_file(const std::string& url, const std::string& output_path) {
    std::cout << "Downloading: " << url << "\n";
    std::cout << "To: " << output_path << "\n";
    
    // Try curl first
    std::string cmd = "curl -L -f -o \"" + output_path + "\" \"" + url + "\" 2>&1";
    if (system(cmd.c_str()) == 0) {
        return true;
    }
    
    std::cout << "curl failed, trying wget...\n";
    
    // Try wget as fallback
    cmd = "wget -O \"" + output_path + "\" \"" + url + "\" 2>&1";
    if (system(cmd.c_str()) == 0) {
        return true;
    }
    
    std::cerr << "Failed to download file (tried curl and wget)\n";
    return false;
}

// Extract an archive to a directory
inline bool extract_archive(const std::string& archive_path, const std::string& output_dir, bool strip_components = true) {
    std::filesystem::create_directories(output_dir);
    
    std::string cmd;
    std::string strip = strip_components ? " --strip-components=1" : "";
    
    if (archive_path.ends_with(".tar.xz")) {
        cmd = "tar -xJf \"" + archive_path + "\" -C \"" + output_dir + "\"" + strip;
    } else if (archive_path.ends_with(".tar.gz") || archive_path.ends_with(".tgz")) {
        cmd = "tar -xzf \"" + archive_path + "\" -C \"" + output_dir + "\"" + strip;
    } else if (archive_path.ends_with(".tar.bz2")) {
        cmd = "tar -xjf \"" + archive_path + "\" -C \"" + output_dir + "\"" + strip;
    } else if (archive_path.ends_with(".zip")) {
        cmd = "unzip -q \"" + archive_path + "\" -d \"" + output_dir + "\"";
    } else if (archive_path.ends_with(".7z")) {
        cmd = "7z x \"" + archive_path + "\" -o\"" + output_dir + "\" -y";
    } else {
        std::cerr << "Unsupported archive format: " << archive_path << "\n";
        return false;
    }
    
    std::cout << "Extracting: " << archive_path << "\n";
    int result = system(cmd.c_str());
    
    if (result != 0) {
        std::cerr << "Extraction failed\n";
        return false;
    }
    
    return true;
}

// Download and extract in one step
inline bool download_and_extract(const std::string& url, const std::string& output_dir, bool strip_components = true) {
    // Determine archive filename from URL
    std::string filename = url.substr(url.find_last_of('/') + 1);
    std::string temp_path = std::filesystem::temp_directory_path() / filename;
    
    if (!download_file(url, temp_path)) {
        return false;
    }
    
    bool success = extract_archive(temp_path, output_dir, strip_components);
    
    // Clean up temporary file
    std::filesystem::remove(temp_path);
    
    return success;
}

} // namespace forma::download
