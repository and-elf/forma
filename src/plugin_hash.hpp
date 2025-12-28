#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace forma {

// Simple FNV-1a hash for compile-time metadata verification
constexpr uint64_t fnv1a_hash(std::string_view str) {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Helper to convert hash to hex string at runtime
inline std::string hash_to_hex(uint64_t hash) {
    char buf[17];
    snprintf(buf, sizeof(buf), "%016lx", hash);
    return std::string(buf);
}

} // namespace forma
