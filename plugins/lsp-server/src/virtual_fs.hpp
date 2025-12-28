#pragma once

#include <string>
#include <string_view>
#include <array>
#include <optional>

namespace forma::vfs {

// Virtual file entry
struct VirtualFile {
    std::string uri;
    std::string content;
    int version = 0;
    bool exists = false;
};

// Simple virtual filesystem for testing
template<size_t MaxFiles = 64>
class VirtualFS {
    std::array<VirtualFile, MaxFiles> files;
    size_t file_count = 0;
    
public:
    constexpr VirtualFS() = default;
    
    // Add or update a file
    bool write_file(std::string_view uri, std::string_view content, int version = 1) {
        // Try to find existing file
        VirtualFile* file = find_file(uri);
        
        if (file) {
            // Update existing
            file->content = std::string(content);
            file->version = version;
            return true;
        }
        
        // Add new file
        if (file_count < MaxFiles) {
            files[file_count].uri = std::string(uri);
            files[file_count].content = std::string(content);
            files[file_count].version = version;
            files[file_count].exists = true;
            file_count++;
            return true;
        }
        
        return false; // No space
    }
    
    // Read a file
    std::optional<std::string_view> read_file(std::string_view uri) const {
        const VirtualFile* file = find_file(uri);
        if (file && file->exists) {
            return file->content;
        }
        return std::nullopt;
    }
    
    // Check if file exists
    bool exists(std::string_view uri) const {
        const VirtualFile* file = find_file(uri);
        return file && file->exists;
    }
    
    // Delete a file
    bool remove_file(std::string_view uri) {
        VirtualFile* file = find_file(uri);
        if (file) {
            file->exists = false;
            return true;
        }
        return false;
    }
    
    // Get file version
    int get_version(std::string_view uri) const {
        const VirtualFile* file = find_file(uri);
        if (file && file->exists) {
            return file->version;
        }
        return 0;
    }
    
    // List all files
    std::array<std::string_view, MaxFiles> list_files() const {
        std::array<std::string_view, MaxFiles> result{};
        size_t count = 0;
        
        for (size_t i = 0; i < file_count && count < MaxFiles; ++i) {
            if (files[i].exists) {
                result[count++] = files[i].uri;
            }
        }
        
        return result;
    }
    
    // Get file count
    size_t count() const {
        size_t active_count = 0;
        for (size_t i = 0; i < file_count; ++i) {
            if (files[i].exists) {
                active_count++;
            }
        }
        return active_count;
    }
    
    // Clear all files
    void clear() {
        for (size_t i = 0; i < file_count; ++i) {
            files[i].exists = false;
        }
        file_count = 0;
    }
    
private:
    VirtualFile* find_file(std::string_view uri) {
        for (size_t i = 0; i < file_count; ++i) {
            if (files[i].uri == uri && files[i].exists) {
                return &files[i];
            }
        }
        return nullptr;
    }
    
    const VirtualFile* find_file(std::string_view uri) const {
        for (size_t i = 0; i < file_count; ++i) {
            if (files[i].uri == uri && files[i].exists) {
                return &files[i];
            }
        }
        return nullptr;
    }
};

// LSP workspace with virtual filesystem
template<typename LSPManager, size_t MaxFiles = 64>
class VirtualWorkspace {
    VirtualFS<MaxFiles> fs;
    LSPManager& lsp_manager;
    
public:
    VirtualWorkspace(LSPManager& manager) : lsp_manager(manager) {}
    
    // Initialize the LSP server
    auto initialize(int process_id = 0, std::string_view root_uri = "file:///workspace") {
        return lsp_manager.initialize(process_id, root_uri);
    }
    
    // Create or update a file and notify LSP
    bool create_file(std::string_view uri, std::string_view content) {
        bool is_new = !fs.exists(uri);
        int version = is_new ? 1 : fs.get_version(uri) + 1;
        
        if (!fs.write_file(uri, content, version)) {
            return false;
        }
        
        if (is_new) {
            // Open document in LSP
            forma::lsp::TextDocumentItem item;
            item.uri = uri;
            item.language_id = "forma";
            item.version = version;
            item.text = *fs.read_file(uri);
            lsp_manager.did_open(item);
        } else {
            // Update document in LSP
            forma::lsp::VersionedTextDocumentIdentifier id;
            id.uri = uri;
            id.version = version;
            lsp_manager.did_change(id, *fs.read_file(uri));
        }
        
        return true;
    }
    
    // Update file content
    bool update_file(std::string_view uri, std::string_view content) {
        if (!fs.exists(uri)) {
            return false;
        }
        
        int version = fs.get_version(uri) + 1;
        fs.write_file(uri, content, version);
        
        forma::lsp::VersionedTextDocumentIdentifier id;
        id.uri = uri;
        id.version = version;
        lsp_manager.did_change(id, *fs.read_file(uri));
        
        return true;
    }
    
    // Delete a file and notify LSP
    bool delete_file(std::string_view uri) {
        if (!fs.exists(uri)) {
            return false;
        }
        
        forma::lsp::TextDocumentIdentifier id;
        id.uri = uri;
        lsp_manager.did_close(id);
        
        return fs.remove_file(uri);
    }
    
    // Get diagnostics for a file
    auto get_diagnostics(std::string_view uri) const {
        return lsp_manager.find_document(uri);
    }
    
    // Read file content
    std::optional<std::string_view> read_file(std::string_view uri) const {
        return fs.read_file(uri);
    }
    
    // Check if file exists
    bool exists(std::string_view uri) const {
        return fs.exists(uri);
    }
    
    // List all files
    auto list_files() const {
        return fs.list_files();
    }
    
    // Get file count
    size_t file_count() const {
        return fs.count();
    }
    
    // Access the filesystem directly
    VirtualFS<MaxFiles>& filesystem() { return fs; }
    const VirtualFS<MaxFiles>& filesystem() const { return fs; }
    
    // Access the LSP manager directly
    LSPManager& lsp() { return lsp_manager; }
    const LSPManager& lsp() const { return lsp_manager; }
};

} // namespace forma::vfs
