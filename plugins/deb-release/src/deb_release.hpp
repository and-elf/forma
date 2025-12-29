#pragma once

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

// Helper function to parse simple config file
inline bool parse_package_config(const std::string& config_path, PackageMetadata& meta) {
    std::ifstream file(config_path);
    if (!file.is_open()) return false;
    
    std::string line;
    std::string current_section;
    std::ostringstream script_content;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line[start] == '#') continue;
        
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        
        // Check for section markers
        if (line == "[postinst]") {
            if (!current_section.empty() && !script_content.str().empty()) {
                if (current_section == "postinst") meta.postinst_script = script_content.str();
                else if (current_section == "prerm") meta.prerm_script = script_content.str();
                else if (current_section == "postrm") meta.postrm_script = script_content.str();
            }
            current_section = "postinst";
            script_content.str("");
            continue;
        } else if (line == "[prerm]") {
            if (!current_section.empty() && !script_content.str().empty()) {
                if (current_section == "postinst") meta.postinst_script = script_content.str();
                else if (current_section == "prerm") meta.prerm_script = script_content.str();
            }
            current_section = "prerm";
            script_content.str("");
            continue;
        } else if (line == "[postrm]") {
            if (!current_section.empty() && !script_content.str().empty()) {
                if (current_section == "postinst") meta.postinst_script = script_content.str();
                else if (current_section == "prerm") meta.prerm_script = script_content.str();
                else if (current_section == "postrm") meta.postrm_script = script_content.str();
            }
            current_section = "postrm";
            script_content.str("");
            continue;
        } else if (line[0] == '[') {
            if (!current_section.empty() && !script_content.str().empty()) {
                if (current_section == "postinst") meta.postinst_script = script_content.str();
                else if (current_section == "prerm") meta.prerm_script = script_content.str();
                else if (current_section == "postrm") meta.postrm_script = script_content.str();
            }
            current_section = "";
            script_content.str("");
            continue;
        }
        
        // Handle script sections
        if (!current_section.empty()) {
            if (!script_content.str().empty()) script_content << "\n";
            script_content << line;
            continue;
        }
        
        // Parse key = value
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim key and value
        key.erase(key.find_last_not_of(" \t") + 1);
        size_t vstart = value.find_first_not_of(" \t\"");
        if (vstart != std::string::npos) {
            value = value.substr(vstart);
        }
        if (!value.empty() && value.back() == '"') value.pop_back();
        
        if (key == "name") meta.name = value;
        else if (key == "version") meta.version = value;
        else if (key == "maintainer") meta.maintainer = value;
        else if (key == "description") meta.description = value;
        else if (key == "architecture") meta.architecture = value;
        else if (key == "section") meta.section = value;
        else if (key == "priority") meta.priority = value;
        else if (key == "homepage") meta.homepage = value;
        else if (key == "copyright") meta.copyright_holder = value;
        else if (key == "license") meta.license = value;
        else if (key == "depends") {
            // Split by comma
            std::istringstream ss(value);
            std::string dep;
            while (std::getline(ss, dep, ',')) {
                size_t s = dep.find_first_not_of(" \t");
                if (s != std::string::npos) {
                    dep = dep.substr(s);
                    size_t e = dep.find_last_not_of(" \t");
                    dep = dep.substr(0, e + 1);
                    meta.dependencies.push_back(dep);
                }
            }
        }
    }
    
    // Handle last section
    if (!current_section.empty() && !script_content.str().empty()) {
        if (current_section == "postinst") meta.postinst_script = script_content.str();
        else if (current_section == "prerm") meta.prerm_script = script_content.str();
        else if (current_section == "postrm") meta.postrm_script = script_content.str();
    }
    
    return true;
}

} // namespace forma::deb
