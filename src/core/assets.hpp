#pragma once
#include "../parser/ir.hpp"
#include <string_view>
#include <array>

namespace forma {

// ============================================================================
// Asset Bundler - Collects and processes forma:// URIs
// ============================================================================

template <size_t MaxAssets = 64>
struct AssetBundler {
    std::array<AssetDecl, MaxAssets> assets{};
    size_t asset_count = 0;
    
    // Determine asset type from file extension
    constexpr AssetDecl::Type get_asset_type(std::string_view path) const {
        // Find the last dot
        size_t dot_pos = path.size();
        for (size_t i = path.size(); i > 0; --i) {
            if (path[i-1] == '.') {
                dot_pos = i - 1;
                break;
            }
        }
        
        if (dot_pos == path.size()) {
            return AssetDecl::Type::Binary; // No extension
        }
        
        auto ext = path.substr(dot_pos + 1);
        
        // Image formats
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || 
            ext == "gif" || ext == "svg") {
            return AssetDecl::Type::Image;
        }
        
        // Font formats
        if (ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2") {
            return AssetDecl::Type::Font;
        }
        
        return AssetDecl::Type::Binary;
    }
    
    // Generate C-safe symbol name from URI
    constexpr void generate_symbol_name(char* buffer, size_t buffer_size, 
                                       std::string_view uri) const {
        size_t pos = 0;
        
        // Add prefix
        const char* prefix = "asset_";
        for (const char* p = prefix; *p && pos < buffer_size - 1; ++p) {
            buffer[pos++] = *p;
        }
        
        // Skip "forma://" prefix
        size_t start = uri.find("://");
        if (start != std::string_view::npos) {
            start += 3;
        } else {
            start = 0;
        }
        
        // Convert path to valid C identifier
        for (size_t i = start; i < uri.size() && pos < buffer_size - 1; ++i) {
            char c = uri[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                buffer[pos++] = (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
            } else {
                buffer[pos++] = '_';
            }
        }
        
        buffer[pos] = '\0';
    }
    
    // Add asset if it's a forma:// URI and not already added
    constexpr bool add_asset(std::string_view uri) {
        // Check if it's a forma:// URI
        if (uri.size() < 8 || uri.substr(0, 8) != "forma://") {
            return false;
        }
        
        // Check if already added
        for (size_t i = 0; i < asset_count; ++i) {
            if (assets[i].uri == uri) {
                return true; // Already exists
            }
        }
        
        if (asset_count >= MaxAssets) {
            return false; // No space
        }
        
        // Create new asset entry
        AssetDecl asset{};
        asset.uri = uri;
        asset.type = get_asset_type(uri);
        
        // Get file path (everything after forma://)
        asset.file_path = uri.substr(8);
        
        // Symbol name will be generated during code generation
        // We store the URI for now
        asset.symbol_name = uri;
        
        assets[asset_count++] = asset;
        return true;
    }
    
    // Scan a value for forma:// URIs
    constexpr void scan_value(const Value& value) {
        if (value.kind == Value::Kind::String || value.kind == Value::Kind::URI) {
            add_asset(value.text);
        }
    }
    
    // Scan an instance for assets
    constexpr void scan_instance(const InstanceDecl& inst) {
        // Scan all properties
        for (size_t i = 0; i < inst.prop_count; ++i) {
            scan_value(inst.properties[i].value);
            if (inst.properties[i].has_preview) {
                scan_value(inst.properties[i].preview_value);
            }
        }
        
        // Scan when blocks
        for (size_t i = 0; i < inst.when_count; ++i) {
            for (size_t j = 0; j < inst.when_stmts[i].assignment_count; ++j) {
                scan_value(inst.when_stmts[i].assignments[j].value);
            }
        }
        
        // Scan animations
        for (size_t i = 0; i < inst.animation_count; ++i) {
            scan_value(inst.animations[i].start_value);
            scan_value(inst.animations[i].end_value);
        }
    }
    
    // Scan entire document for assets
    template <typename DocType>
    constexpr void scan_document(const DocType& doc) {
        // Scan all instances
        for (size_t i = 0; i < doc.instances.count; ++i) {
            scan_instance(doc.instances.get(i));
        }
    }
};

// Collect all assets from a document
template <typename DocType>
constexpr AssetBundler<64> collect_assets(const DocType& doc) {
    AssetBundler<64> bundler;
    bundler.scan_document(doc);
    return bundler;
}

} // namespace forma
