#pragma once

#include <functional>
#include <optional>
#include <string>
#include <fstream>
#include <filesystem>
#include <system_error>
#include "../fs/i_file_system.hpp"
#include "write_stream.hpp"

namespace forma::io {

// Open a file for reading; returns file contents or std::nullopt on failure
using OpenReadFn = std::function<std::optional<std::string>(const std::string& path)>;

// Open a file for writing with a full contents buffer; returns true on success
using OpenWriteFn = std::function<bool(const std::string& path, const std::string& contents)>;

// Open a write stream (RAII) for chunked writes; return nullptr on failure
using OpenWriteStreamFn = std::function<forma::io::WriteStreamPtr(const std::string& path)>;

// Create directories for a path; returns true on success
using CreateDirsFn = std::function<bool(const std::string& path)>;

struct StreamIO {
    OpenReadFn open_read;
    OpenWriteFn open_write;
    OpenWriteStreamFn open_write_stream;
    CreateDirsFn create_dirs;

    StreamIO() = default;

    static StreamIO defaults() {
        StreamIO s;
        s.open_read = default_open_read;
        s.open_write = default_open_write;
        s.open_write_stream = default_open_write_stream;
        s.create_dirs = default_create_dirs;
        return s;
    }

    // Create StreamIO backed by an IFileSystem implementation
    static StreamIO from_filesystem(forma::fs::IFileSystem& fs) {
        StreamIO s;
        s.open_read = [&fs](const std::string& path) -> std::optional<std::string> {
            try {
                if (!fs.exists(path)) return std::nullopt;
                return fs.read_file(path);
            } catch (...) {
                return std::nullopt;
            }
        };
        s.open_write = [&fs](const std::string& path, const std::string& contents) -> bool {
            try {
                // Ensure parent directories exist if the filesystem supports it
                fs.write_file(path, contents);
                return true;
            } catch (...) {
                return false;
            }
        };
        s.open_write_stream = [&fs](const std::string& path) -> forma::io::WriteStreamPtr {
            try {
                return fs.open_write_stream(path);
            } catch (...) {
                return nullptr;
            }
        };
        s.create_dirs = [&fs](const std::string& path) -> bool {
            try {
                fs.create_dirs(path);
                return true;
            } catch (...) {
                return false;
            }
        };
        return s;
    }

    // Default implementations that use the host filesystem (std::ifstream/ofstream)
    static std::optional<std::string> default_open_read(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return std::nullopt;
        return std::string((std::istreambuf_iterator<char>(f)), {});
    }

    static bool default_open_write(const std::string& path, const std::string& contents) {
        try {
            // Ensure parent directories exist
            auto parent = std::filesystem::path(path).parent_path();
            if (!parent.empty()) std::filesystem::create_directories(parent);
            std::ofstream f(path, std::ios::binary);
            if (!f) return false;
            f << contents;
            return true;
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    static forma::io::WriteStreamPtr default_open_write_stream(const std::string& path) {
        try {
            auto parent = std::filesystem::path(path).parent_path();
            if (!parent.empty()) std::filesystem::create_directories(parent);
            struct OfsStream : public forma::io::IWriteStream {
                std::ofstream ofs;
                OfsStream(const std::string& p) : ofs(p, std::ios::binary) {}
                std::size_t write(const void* data, std::size_t len) override {
                    ofs.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
                    return len;
                }
            };
            return std::make_unique<OfsStream>(path);
        } catch (...) {
            return nullptr;
        }
    }

    static bool default_create_dirs(const std::string& path) {
        try {
            std::filesystem::create_directories(path);
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace forma::io
