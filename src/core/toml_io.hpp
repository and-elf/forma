#pragma once

#include "core/fs/i_file_system.hpp"
#include "toml/toml.hpp"
#include <optional>
#include <string>

namespace forma::core {

// Read a TOML file from an IFileSystem into a string. Returns true on success.
inline bool read_toml_file(forma::fs::IFileSystem& fs, const std::string& path, std::string& out) {
    try {
        if (!fs.exists(path)) return false;
        out = fs.read_file(path);
        return true;
    } catch (...) {
        return false;
    }
}

// Parse a TOML document from an IFileSystem path. Returns std::optional<Document>.
template<size_t MaxTables = 16>
inline std::optional<forma::toml::Document<MaxTables>> parse_toml_from_fs(forma::fs::IFileSystem& fs, const std::string& path) {
    std::string content;
    if (!read_toml_file(fs, path, content)) return std::nullopt;
    // Use the parser
    auto doc = forma::toml::parse<MaxTables>(content);
    return doc;
}

// Helper to get a table by name from a TOML file in fs
template<size_t MaxTables = 16>
inline const forma::toml::Table<32>* read_toml_table(forma::fs::IFileSystem& fs, const std::string& path, const std::string& table_name) {
    auto doc_opt = parse_toml_from_fs<MaxTables>(fs, path);
    if (!doc_opt) return nullptr;
    auto& doc = *doc_opt;
    return doc.get_table(table_name);
}

} // namespace forma::core
