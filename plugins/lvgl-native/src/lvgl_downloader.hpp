#pragma once

#include <archive.hpp>
#include <download.hpp>
#include <core/config.hpp>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace forma::lvgl {

inline namespace detail {
    static std::string get_local_lvgl_path_impl() {
        auto config = forma::config::load_project_config(".");
        std::string toolchain_dir = forma::config::get_toolchain_dir(config);
        return toolchain_dir + "/lvgl";
    }

    static bool download_lvgl_impl(const std::string& version) {
        return download_lvgl_impl(version, nullptr);
    }

    // Host-aware variant
    static bool download_lvgl_impl(const std::string& version, forma::HostContext* host) {
        std::string install_path = get_local_lvgl_path_impl();
        forma::fs::RealFileSystem realfs;
        realfs.create_dirs(install_path);

        std::string lvgl_version = version;
        std::string url = "https://github.com/lvgl/lvgl/archive/refs/tags/v" + lvgl_version + ".tar.gz";

        std::string archive_path = install_path + "/lvgl.tar.gz";
        forma::download::DownloadResult result;
        if (host) {
            // call host-aware wrapper
            bool ok = ::forma_download_host(host, url.c_str(), archive_path.c_str(), nullptr);
            result.success = ok;
        } else {
            result = forma::download::download_file(url, archive_path);
        }
        if (!result.success) {
            std::cerr << "Failed to download LVGL: " << result.error_message << "\n";
            return false;
        }

        forma::archive::ExtractOptions extract_opts;
        extract_opts.strip_components = 1;
        auto extract_result = forma::archive::extract_archive(archive_path.c_str(), install_path.c_str(), extract_opts);
        forma::fs::RealFileSystem realfs2;
        if (realfs2.exists(archive_path)) std::remove(archive_path.c_str());
        if (!extract_result.success) {
            std::cerr << "Failed to extract LVGL: " << extract_result.error_message << "\n";
            return false;
        }

        std::cout << "LVGL v" << lvgl_version << " installed to: " << install_path << "\n";
        return true;
    }
}

inline std::string ensure_lvgl_available(const std::string& version = "9.2.2") {
    auto local = detail::get_local_lvgl_path_impl();
    forma::fs::RealFileSystem realfs3;
    if (realfs3.exists(local + "/lvgl.h")) return local;
    std::cout << "LVGL not found. Downloading...\n";
    if (!detail::download_lvgl_impl(version)) return "";
    return local;
}

inline bool download_lvgl(const std::string& version) {
    return detail::download_lvgl_impl(version);
}

inline std::string get_local_lvgl_path() {
    return detail::get_local_lvgl_path_impl();
}

} // namespace forma::lvgl
