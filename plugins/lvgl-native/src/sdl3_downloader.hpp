#pragma once

#include <archive.hpp>
#include <download.hpp>
#include <core/config.hpp>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace forma::sdl3 {

inline namespace detail {
    static std::string get_local_sdl3_path_impl() {
        auto config = forma::config::load_project_config(".");
        std::string toolchain_dir = forma::config::get_toolchain_dir(config);
        return toolchain_dir + "/sdl3";
    }

    static bool download_sdl3_impl(const std::string& version) {
        std::string install_path = get_local_sdl3_path_impl();
        std::filesystem::create_directories(install_path);

        std::string sdl3_version = version;
        std::string url = "https://github.com/libsdl-org/SDL/releases/download/preview-" +
                          sdl3_version + "/SDL3-" + sdl3_version + ".tar.gz";

        std::string archive_path = install_path + "/sdl3.tar.gz";
        auto result = forma::download::download_file(url, archive_path);
        if (!result.success) {
            std::cerr << "Failed to download SDL3: " << result.error_message << "\n";
            return false;
        }

        std::string temp_dir = install_path + "/SDL3-" + sdl3_version;
        forma::archive::ExtractOptions extract_opts;
        extract_opts.strip_components = 1;
        auto extract_result = forma::archive::extract_archive(archive_path.c_str(), temp_dir.c_str(), extract_opts);
        std::filesystem::remove(archive_path);
        if (!extract_result.success) {
            std::cerr << "Failed to extract SDL3: " << extract_result.error_message << "\n";
            return false;
        }

        std::cout << "Building SDL3 (this may take a few minutes)...\n";
        std::string build_cmd = "cd \"" + temp_dir + "\" && "
                               "cmake -B build -DCMAKE_INSTALL_PREFIX=\"" + install_path + "\" && "
                               "cmake --build build && "
                               "cmake --install build";

        int build_result = system(build_cmd.c_str());
        std::filesystem::remove_all(temp_dir);
        if (build_result != 0) {
            std::cerr << "Failed to build SDL3\n";
            return false;
        }

        std::cout << "SDL3 installed to: " << install_path << "\n";
        return true;
    }
}

// Public API - inline functions to be usable from headers
inline std::string ensure_sdl3_available(const std::string& version = "3.1.6") {
    // Try pkg-config first
    if (system("pkg-config --exists sdl3 2>/dev/null") == 0) return "sdl3";

    auto local = detail::get_local_sdl3_path_impl();
    if (std::filesystem::exists(local + "/include/SDL3/SDL.h")) return local;

    std::cout << "SDL3 not found. Downloading...\n";
    if (!detail::download_sdl3_impl(version)) return "";
    return local;
}

inline bool download_sdl3(const std::string& version) {
    return detail::download_sdl3_impl(version);
}

inline std::string get_local_sdl3_path() {
    return detail::get_local_sdl3_path_impl();
}

} // namespace forma::sdl3
