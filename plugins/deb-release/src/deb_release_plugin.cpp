#include "deb_release.hpp"
#include <iostream>
#include <cstring>

extern "C" {

const char* get_plugin_name() {
    return "deb-release";
}

const char* get_plugin_version() {
    return "0.1.0";
}

const char* get_plugin_description() {
    return "Generates Debian .deb packages from built applications";
}

// Main entry point: build_dir contains the application files,
// config_file contains package metadata
bool create_debian_package(const char* build_dir, const char* source_dir, const char* config_file) {
    if (!build_dir || !source_dir || !config_file) {
        std::cerr << "Error: Invalid arguments\n";
        return false;
    }
    
    forma::deb::PackageMetadata meta;
    
    // Load configuration
    if (!forma::deb::parse_package_config(config_file, meta)) {
        std::cerr << "Error: Failed to load package configuration from " << config_file << "\n";
        return false;
    }
    
    std::cout << "Building Debian package: " << meta.name << " v" << meta.version << "\n";
    
    forma::deb::DebianPackageBuilder builder(build_dir, source_dir);
    builder.set_metadata(meta);
    
    if (!builder.build_package()) {
        std::cerr << "Error: Failed to build package structure\n";
        return false;
    }
    
    std::cout << "Package structure created in: " << build_dir << "\n";
    std::cout << "To build .deb file, run: dpkg-deb --build " << build_dir << " " 
              << builder.get_package_filename() << "\n";
    
    return true;
}

} // extern "C"
