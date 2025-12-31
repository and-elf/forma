#pragma once

#include <toml/toml.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>

namespace forma::deb {

struct PackageMetadata {
    std::string name = "forma-app";
    std::string version = "1.0.0";
    std::string maintainer = "Unknown <dev@example.com>";
    std::string description = "Application built with Forma";
    std::string architecture = "amd64";
    std::string section = "misc";
    std::string priority = "optional";
    std::string homepage;
    std::vector<std::string> dependencies;
    std::string postinst_script;
    std::string prerm_script;
    std::string postrm_script;
    std::string preinst_script;
    std::string copyright_holder;
    std::string license = "MIT";
    
    // Load metadata from TOML document
    void load_from_toml(const toml::Table<32>& table) {
        if (auto val = table.get_string("name")) name = std::string(*val);
        if (auto val = table.get_string("version")) version = std::string(*val);
        if (auto val = table.get_string("maintainer")) maintainer = std::string(*val);
        if (auto val = table.get_string("description")) description = std::string(*val);
        if (auto val = table.get_string("architecture")) architecture = std::string(*val);
        if (auto val = table.get_string("section")) section = std::string(*val);
        if (auto val = table.get_string("priority")) priority = std::string(*val);
        if (auto val = table.get_string("homepage")) homepage = std::string(*val);
        if (auto val = table.get_string("copyright")) copyright_holder = std::string(*val);
        if (auto val = table.get_string("license")) license = std::string(*val);
        
        // Handle dependencies array
        // Note: TOML parser would need to support this, for now keep simple config fallback
    }
    
    // Apply CLI overrides (values from command-line arguments)
    void apply_overrides(const std::string& cli_name, 
                        const std::string& cli_version,
                        const std::string& cli_maintainer,
                        const std::string& cli_description) {
        if (!cli_name.empty()) name = cli_name;
        if (!cli_version.empty()) version = cli_version;
        if (!cli_maintainer.empty()) maintainer = cli_maintainer;
        if (!cli_description.empty()) description = cli_description;
    }
};

class DebianPackageBuilder {
private:
    PackageMetadata metadata;
    std::string build_dir;
    std::string source_dir;
    
    bool create_directory(const std::string& path) {
        return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
    }
    
    bool write_file(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file) return false;
        file << content;
        return true;
    }
    
    bool make_executable(const std::string& path) {
        return chmod(path.c_str(), 0755) == 0;
    }

public:
    DebianPackageBuilder(const std::string& build_dir, const std::string& source_dir)
        : build_dir(build_dir), source_dir(source_dir) {}
    
    bool generate_control_file() {
        std::ostringstream ss;
        ss << "Package: " << metadata.name << "\n";
        ss << "Version: " << metadata.version << "\n";
        ss << "Section: " << metadata.section << "\n";
        ss << "Priority: " << metadata.priority << "\n";
        ss << "Architecture: " << metadata.architecture << "\n";
        ss << "Maintainer: " << metadata.maintainer << "\n";
        
        if (!metadata.dependencies.empty()) {
            ss << "Depends: ";
            for (size_t i = 0; i < metadata.dependencies.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << metadata.dependencies[i];
            }
            ss << "\n";
        }
        
        if (!metadata.homepage.empty()) {
            ss << "Homepage: " << metadata.homepage << "\n";
        }
        
        ss << "Description: " << metadata.description << "\n";
        
        return write_file(build_dir + "/DEBIAN/control", ss.str());
    }
    
    bool generate_postinst() {
        if (metadata.postinst_script.empty()) return true;
        
        std::ostringstream ss;
        ss << "#!/bin/bash\n";
        ss << "set -e\n\n";
        ss << metadata.postinst_script << "\n";
        ss << "\nexit 0\n";
        
        std::string path = build_dir + "/DEBIAN/postinst";
        return write_file(path, ss.str()) && make_executable(path);
    }
    
    bool generate_prerm() {
        if (metadata.prerm_script.empty()) return true;
        
        std::ostringstream ss;
        ss << "#!/bin/bash\n";
        ss << "set -e\n\n";
        ss << metadata.prerm_script << "\n";
        ss << "\nexit 0\n";
        
        std::string path = build_dir + "/DEBIAN/prerm";
        return write_file(path, ss.str()) && make_executable(path);
    }
    
    bool generate_copyright() {
        std::ostringstream ss;
        ss << "Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/\n";
        ss << "Upstream-Name: " << metadata.name << "\n";
        
        if (!metadata.homepage.empty()) {
            ss << "Source: " << metadata.homepage << "\n";
        }
        
        ss << "\nFiles: *\n";
        ss << "Copyright: " << metadata.copyright_holder << "\n";
        ss << "License: " << metadata.license << "\n";
        
        std::string doc_dir = build_dir + "/usr/share/doc/" + metadata.name;
        create_directory(build_dir + "/usr");
        create_directory(build_dir + "/usr/share");
        create_directory(build_dir + "/usr/share/doc");
        create_directory(doc_dir);
        return write_file(doc_dir + "/copyright", ss.str());
    }
    
    bool build_package() {
        // Create DEBIAN directory
        if (!create_directory(build_dir + "/DEBIAN")) return false;
        
        // Generate all control files
        if (!generate_control_file()) return false;
        if (!generate_postinst()) return false;
        if (!generate_prerm()) return false;
        if (!generate_copyright()) return false;
        
        return true;
    }
    
    std::string get_package_filename() const {
        return metadata.name + "_" + metadata.version + "_" + metadata.architecture + ".deb";
    }
    
    void set_metadata(const PackageMetadata& meta) {
        metadata = meta;
    }
    
    const PackageMetadata& get_metadata() const {
        return metadata;
    }
};

// Load package metadata from TOML file
// Supports both [package] and [deploy] tables
inline bool load_package_metadata(const std::string& toml_path, PackageMetadata& meta) {
    // Read TOML file
    std::ifstream file(toml_path);
    if (!file.is_open()) return false;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Parse TOML using the constexpr parse function
    auto doc = toml::parse<16>(content);
    
    // Try [package] table first (standard project metadata)
    if (auto table = doc.get_table("package")) {
        meta.load_from_toml(*table);
    }
    
    // Try [deploy] table second (deployment-specific overrides)
    if (auto table = doc.get_table("deploy")) {
        meta.load_from_toml(*table);
    }
    
    // Handle script sections: [deploy.scripts]
    if (auto deploy_table = doc.get_table("deploy")) {
        // TODO: Support nested tables when TOML parser supports it
        // For now, scripts can be inline strings in [deploy]
        if (auto val = deploy_table->get_string("postinst")) {
            meta.postinst_script = std::string(*val);
        }
        if (auto val = deploy_table->get_string("prerm")) {
            meta.prerm_script = std::string(*val);
        }
        if (auto val = deploy_table->get_string("postrm")) {
            meta.postrm_script = std::string(*val);
        }
        if (auto val = deploy_table->get_string("preinst")) {
            meta.preinst_script = std::string(*val);
        }
    }
    
    return true;
}

} // namespace forma::deb
