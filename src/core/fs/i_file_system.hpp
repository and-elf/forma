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

namespace forma::fs {

struct IFileSystem {
    virtual ~IFileSystem() = default;

    virtual bool exists(std::string_view path) const = 0;
    virtual void create_dirs(std::string_view path) = 0;
    virtual void write_file(std::string_view path, std::string_view contents) = 0;
    virtual std::string read_file(std::string_view path) = 0;
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
        std::ofstream(std::string(p)) << c;
    }

    std::string read_file(std::string_view p) override {
        std::ifstream f(std::string(p), std::ios::binary);
        return {std::istreambuf_iterator<char>(f), {}};
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
};

} // namespace forma::fs
