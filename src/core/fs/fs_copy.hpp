#pragma once

#include "i_file_system.hpp"
#include <filesystem>
#include <fstream>
#include <system_error>

namespace forma::fs {

// Copy files from an IFileSystem under `fs_root` into a disk directory `disk_root`.
// Returns true on success.
inline bool copy_fs_to_disk(IFileSystem& fs, const std::string& fs_root, const std::string& disk_root) {
    try {
        std::string base = fs_root;
        if (!base.empty() && base.back() != '/') base.push_back('/');

        auto files = fs.list_recursive(fs_root);
        for (const auto& f : files) {
            // Ensure file path starts with base
            if (base.empty() || f.rfind(base, 0) == 0) {
                std::string rel = f.substr(base.size());
                std::filesystem::path dest = std::filesystem::path(disk_root) / rel;
                if (!dest.has_parent_path()) continue;
                std::filesystem::create_directories(dest.parent_path());
                std::ofstream out(dest, std::ios::binary);
                if (!out) return false;
                out << fs.read_file(f);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

// Copy files from disk directory `disk_root` into an IFileSystem under `fs_root`.
inline bool copy_disk_to_fs(const std::string& disk_root, IFileSystem& fs, const std::string& fs_root) {
    try {
        std::filesystem::path root(disk_root);
        if (!std::filesystem::exists(root)) return true; // nothing to copy

        for (auto& e : std::filesystem::recursive_directory_iterator(root)) {
            if (!e.is_regular_file()) continue;
            auto rel = std::filesystem::relative(e.path(), root).string();
            std::string dest = fs_root;
            if (!dest.empty() && dest.back() != '/') dest.push_back('/');
            dest += rel;
            // Ensure parent dirs in fs
            std::filesystem::path parent = e.path().parent_path();
            // create parent path string for fs.create_dirs
            auto dest_parent = std::filesystem::path(dest).parent_path().string();
            if (!dest_parent.empty()) fs.create_dirs(dest_parent);
            std::ifstream inf(e.path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(inf)), {});
            fs.write_file(dest, content);
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace forma::fs
