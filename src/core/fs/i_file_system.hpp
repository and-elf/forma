#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <memory>
#include <sstream>
#include "../io/write_stream.hpp"

namespace forma::fs {

struct IFileSystem {
    virtual ~IFileSystem() = default;

    virtual bool exists(std::string_view path) const = 0;
    virtual void create_dirs(std::string_view path) = 0;
    virtual void write_file(std::string_view path, std::string_view contents) = 0;
    virtual std::string read_file(std::string_view path) = 0;
    // Return a list of file paths under `path` recursively (including path itself if file)
    virtual std::vector<std::string> list_recursive(std::string_view path) const = 0;
    // Open a write stream for path; returns nullptr on failure. The returned
    // stream will flush on destruction (RAII). Implementations may create
    // parent directories as needed.
    virtual std::unique_ptr<forma::io::IWriteStream> open_write_stream(std::string_view path) = 0;
};

class RealFileSystem final : public IFileSystem {
public:
    bool exists(std::string_view p) const override {
        return std::filesystem::exists(std::string(p));
    }

    void create_dirs(std::string_view p) override {
        std::filesystem::create_directories(std::filesystem::path(std::string(p)));
    }

    void write_file(std::string_view p, std::string_view c) override {
        std::ofstream ofs(std::string(p), std::ios::binary);
        ofs << c;
    }

    std::string read_file(std::string_view p) override {
        std::ifstream f(std::string(p), std::ios::binary);
        return {std::istreambuf_iterator<char>(f), {}};
    }

    std::vector<std::string> list_recursive(std::string_view path) const override {
        std::vector<std::string> out;
        std::filesystem::path p{std::string(path)};
        if (!std::filesystem::exists(p)) return out;
        if (std::filesystem::is_regular_file(p)) {
            out.push_back(p.string());
            return out;
        }
        for (const auto& e : std::filesystem::recursive_directory_iterator(p)) {
            if (e.is_regular_file()) out.push_back(e.path().string());
        }
        return out;
    }
    std::unique_ptr<forma::io::IWriteStream> open_write_stream(std::string_view path) override {
        // Ensure parent dirs exist
        auto parent = std::filesystem::path(std::string(path)).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        // Create an ofstream wrapped in a small IWriteStream
        struct OfsStream : public forma::io::IWriteStream {
            std::ofstream ofs;
            OfsStream(const std::string& p) : ofs(p, std::ios::binary) {}
            std::size_t write(const void* data, std::size_t len) override {
                ofs.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
                return static_cast<std::size_t>(len);
            }
        };
        try {
            return std::make_unique<OfsStream>(std::string(path));
        } catch (...) {
            return nullptr;
        }
    }
};

class MemoryFileSystem final : public IFileSystem {
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> files_;
    std::unordered_set<std::string> dirs_;

public:
    bool exists(std::string_view p) const override {
        std::lock_guard<std::mutex> lk(mu_);
        std::string s(p);
        return files_.find(s) != files_.end() || dirs_.find(s) != dirs_.end();
    }

    void create_dirs(std::string_view p) override {
        std::lock_guard<std::mutex> lk(mu_);
        dirs_.insert(std::string(p));
    }

    void write_file(std::string_view p, std::string_view c) override {
        std::lock_guard<std::mutex> lk(mu_);
        files_[std::string(p)] = std::string(c);
    }

    std::string read_file(std::string_view p) override {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = files_.find(std::string(p));
        if (it == files_.end()) throw std::out_of_range("file not found");
        return it->second;
    }

    std::vector<std::string> list_recursive(std::string_view path) const override {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<std::string> out;
        std::string base(path);
        // If base is a file, return it
        if (files_.find(base) != files_.end()) {
            out.push_back(base);
            return out;
        }
        // Otherwise, collect files that start with base + '/'
        std::string prefix = base;
        if (!prefix.empty() && prefix.back() != '/') prefix.push_back('/');
        for (const auto& kv : files_) {
            if (kv.first.rfind(prefix, 0) == 0) {
                out.push_back(kv.first);
            }
        }
        return out;
    }
    std::unique_ptr<forma::io::IWriteStream> open_write_stream(std::string_view path) override {
        // Return a stream that appends into the in-memory file buffer.
        struct MemStream : public forma::io::IWriteStream {
            std::string key;
            MemoryFileSystem* fs;
            MemStream(MemoryFileSystem* fs_, std::string k) : key(std::move(k)), fs(fs_) {}
            std::size_t write(const void* data, std::size_t len) override {
                std::lock_guard<std::mutex> lk(fs->mu_);
                auto& slot = fs->files_[key];
                slot.append(reinterpret_cast<const char*>(data), len);
                return len;
            }
        };
        // Ensure parent dir exists semantically
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto sp = std::string(path);
            auto parent = std::filesystem::path(sp).parent_path().string();
            if (!parent.empty()) dirs_.insert(parent);
        }
        return std::make_unique<MemStream>(this, std::string(path));
    }
};

} // namespace forma::fs
