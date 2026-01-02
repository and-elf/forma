#include "archive.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <filesystem>
#include <cstring>
#include "../../src/core/host_context.hpp"
#include "../../src/core/fs/fs_copy.hpp"

namespace forma::archive {

using std::filesystem::path;

// Helper to strip leading path components
static std::string strip_path_components(const std::string& path, int components) {
    if (components <= 0) return path;
    
    fs::path p(path);
    auto it = p.begin();
    
    // Skip the first N components
    for (int i = 0; i < components && it != p.end(); ++i) {
        ++it;
    }
    
    // Reconstruct path from remaining components
    fs::path result;
    for (; it != p.end(); ++it) {
        result /= *it;
    }
    
    return result.string();
}

ExtractResult extract_archive(
    const std::string& archive_path,
    const std::string& dest_dir,
    const ExtractOptions& options
) {
    ExtractResult result;
    
    // Create destination directory if requested
    if (options.create_dest_dir) {
        try {
            std::filesystem::create_directories(dest_dir);
        } catch (const std::filesystem::filesystem_error& e) {
            result.error_message = std::string("Failed to create destination directory: ") + e.what();
            return result;
        }
    }
    
    // Open the archive for reading
    struct archive* a = archive_read_new();
    if (!a) {
        result.error_message = "Failed to create archive reader";
        return result;
    }
    
    // Enable all supported formats and filters
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    
    // Open the file
    int r = archive_read_open_filename(a, archive_path.c_str(), 10240);
    if (r != ARCHIVE_OK) {
        result.error_message = std::string("Failed to open archive: ") + archive_error_string(a);
        archive_read_free(a);
        return result;
    }
    
    // First pass: count total files for progress
    size_t total_files = 0;
    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        total_files++;
        archive_read_data_skip(a);
    }
    
    // Reopen for extraction
    archive_read_free(a);
    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    archive_read_open_filename(a, archive_path.c_str(), 10240);
    
    // Create archive writer for extraction
    struct archive* ext = archive_write_disk_new();
    int write_flags = ARCHIVE_EXTRACT_TIME |
                      ARCHIVE_EXTRACT_PERM |
                      ARCHIVE_EXTRACT_ACL |
                      ARCHIVE_EXTRACT_FFLAGS;
    
    if (options.overwrite) {
        write_flags |= ARCHIVE_EXTRACT_UNLINK;
    }
    
    archive_write_disk_set_options(ext, write_flags);
    
    // Extract each entry
    size_t current_file = 0;
    while (true) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r != ARCHIVE_OK) {
            result.error_message = std::string("Error reading archive header: ") + archive_error_string(a);
            archive_read_free(a);
            archive_write_free(ext);
            return result;
        }
        
        // Get the entry pathname
        const char* current_file_path = archive_entry_pathname(entry);
        if (!current_file_path) {
            continue;
        }
        
        // Strip path components if requested
        std::string stripped_path = strip_path_components(current_file_path, options.strip_components);
        if (stripped_path.empty()) {
            continue; // Path was completely stripped
        }
        
        // Construct full destination path
            std::filesystem::path dest_path = std::filesystem::path(dest_dir) / stripped_path;
        archive_entry_set_pathname(entry, dest_path.string().c_str());
        
        // Write the entry
        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "Warning: " << archive_error_string(ext) << std::endl;
        } else {
            // Copy data
            const void* buff;
            size_t size;
            la_int64_t offset;
            
            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) {
                    break;
                }
                if (r != ARCHIVE_OK) {
                    result.error_message = std::string("Error reading data: ") + archive_error_string(a);
                    archive_read_free(a);
                    archive_write_free(ext);
                    return result;
                }
                
                r = archive_write_data_block(ext, buff, size, offset);
                if (r != ARCHIVE_OK) {
                    result.error_message = std::string("Error writing data: ") + archive_error_string(ext);
                    archive_read_free(a);
                    archive_write_free(ext);
                    return result;
                }
                
                result.bytes_extracted += size;
            }
        }
        
        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK) {
            std::cerr << "Warning: " << archive_error_string(ext) << std::endl;
        }
        
        result.files_extracted++;
        current_file++;
        
        // Call progress callback if provided
        if (options.progress_callback) {
            options.progress_callback(current_file, total_files);
        }
    }
    
    // Cleanup
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    
    result.success = true;
    return result;
}

std::vector<std::string> list_archive(const std::string& archive_path) {
    std::vector<std::string> files;
    
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    
    int r = archive_read_open_filename(a, archive_path.c_str(), 10240);
    if (r != ARCHIVE_OK) {
        std::cerr << "Failed to open archive: " << archive_error_string(a) << std::endl;
        archive_read_free(a);
        return files;
    }
    
    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (pathname) {
            files.push_back(pathname);
        }
        archive_read_data_skip(a);
    }
    
    archive_read_close(a);
    archive_read_free(a);
    
    return files;
}

ArchiveFormat detect_format(const std::string& archive_path) {
    // Simple detection based on file extension
    std::filesystem::path p(archive_path);
    std::string ext = p.extension().string();
    std::string stem = p.stem().string();
    
    if (ext == ".gz" && stem.ends_with(".tar")) return ArchiveFormat::TarGz;
    if (ext == ".tgz") return ArchiveFormat::TarGz;
    if (ext == ".bz2" && stem.ends_with(".tar")) return ArchiveFormat::TarBz2;
    if (ext == ".tbz2") return ArchiveFormat::TarBz2;
    if (ext == ".xz" && stem.ends_with(".tar")) return ArchiveFormat::TarXz;
    if (ext == ".txz") return ArchiveFormat::TarXz;
    if (ext == ".zip") return ArchiveFormat::Zip;
    if (ext == ".tar") return ArchiveFormat::Tar;
    if (ext == ".7z") return ArchiveFormat::SevenZip;
    
    return ArchiveFormat::Auto;
}

} // namespace forma::archive

// Host-aware C wrappers
extern "C" {

bool forma_extract_from_memory_host(void* host_ptr, const char* data, size_t len, const std::string& dest_dir, int strip_components) {
    if (!host_ptr || !data || len == 0) return false;
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!host || !host->filesystem) return false;

    struct archive* a = archive_read_new();
    if (!a) return false;
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    int r = archive_read_open_memory(a, data, len);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return false;
    }

    struct archive* ext = archive_write_disk_new();
    int write_flags = ARCHIVE_EXTRACT_TIME |
                      ARCHIVE_EXTRACT_PERM |
                      ARCHIVE_EXTRACT_ACL |
                      ARCHIVE_EXTRACT_FFLAGS;
    archive_write_disk_set_options(ext, write_flags);

    struct archive_entry* entry;
    while (true) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
            archive_read_free(a);
            archive_write_free(ext);
            return false;
        }

        const char* pathname = archive_entry_pathname(entry);
        if (!pathname) continue;

        // Handle strip components
        std::string dest_path = std::string(pathname);
        if (strip_components > 0) {
            std::filesystem::path p(dest_path);
            std::filesystem::path out;
            int skip = strip_components;
            for (auto it = p.begin(); it != p.end(); ++it) {
                if (skip > 0) { --skip; continue; }
                out /= *it;
            }
            dest_path = out.string();
            if (dest_path.empty()) continue;
        }

        // Stream entry data directly into the host's StreamIO write stream if available
        std::string full = dest_dir;
        if (!full.empty() && full.back() != '/') full.push_back('/');
        full += dest_path;

        // Ensure parent directories
        try {
            auto parent = std::filesystem::path(full).parent_path().string();
            if (!parent.empty()) host->filesystem->create_dirs(parent);
        } catch(...) {}

        auto writer = host->stream_io.open_write_stream(full);
        if (!writer) {
            // Fallback: try writing via write_file by buffering
            std::string buffer;
            const void* buff;
            size_t size;
            la_int64_t offset;
            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) break;
                if (r != ARCHIVE_OK) {
                    archive_read_free(a);
                    archive_write_free(ext);
                    return false;
                }
                buffer.append(static_cast<const char*>(buff), size);
            }
            try {
                host->filesystem->write_file(full, buffer);
            } catch(...) {}
        } else {
            const void* buff;
            size_t size;
            la_int64_t offset;
            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) break;
                if (r != ARCHIVE_OK) {
                    archive_read_free(a);
                    archive_write_free(ext);
                    return false;
                }
                writer->write(buff, size);
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return true;
}

} // extern "C"

// Host-aware C wrappers
extern "C" {

bool forma_extract_host(void* host_ptr, const char* archive_path_c, const char* dest_dir_c, const void* /*opts*/) {
    if (!host_ptr || !archive_path_c || !dest_dir_c) return false;
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    if (!host || !host->filesystem) return false;

    namespace fs = std::filesystem;
    fs::path tmp = fs::temp_directory_path() / ("forma_extract_" + std::to_string(std::time(nullptr)));
    try { fs::create_directories(tmp); } catch(...) { return false; }

    // Use existing extract_archive to extract into tmp
    forma::archive::ExtractOptions opts;
    bool ok = forma::archive::extract_archive(std::string(archive_path_c), tmp.string(), opts);
    if (!ok) {
        try { fs::remove_all(tmp); } catch(...) {}
        return false;
    }

    bool copied = forma::fs::copy_disk_to_fs(tmp.string(), *host->filesystem, std::string(dest_dir_c));

    try { fs::remove_all(tmp); } catch(...) {}
    return copied;
}

} // extern "C"
