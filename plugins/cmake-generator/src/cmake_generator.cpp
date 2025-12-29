#include "cmake_generator.hpp"
#include "cmake_downloader.hpp"
#include "../../../src/core/toolchain.hpp"
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>

// Global instance
std::unique_ptr<CMakeGenerator> g_cmake_generator;

// Cached cmake path
static std::string g_cmake_path;

// Cached compiler paths for each target
static std::map<std::string, std::string> g_compiler_paths;

// VTable implementation
BuildVTable cmake_build_vtable = {
    cmake_vtable::init,
    cmake_vtable::compile,
    cmake_vtable::link,
    cmake_vtable::clean,
    cmake_vtable::get_diagnostics,
    cmake_vtable::shutdown
};

CMakeGenerator::CMakeGenerator() 
    : context_(nullptr), initialized_(false) {
    // Set defaults
    config_.cmake_minimum_version = "3.20";
    config_.cxx_standard = "20";
    config_.generator = "Ninja";
    config_.build_type = "Release";
    config_.project_name = "FormaProject";
    config_.target_name = "app";
    config_.output_dir = "build";
}

CMakeGenerator::~CMakeGenerator() {
    if (initialized_) {
        shutdown();
    }
}

void CMakeGenerator::init(BuildContext* ctx) {
    context_ = ctx;
    initialized_ = true;
    clear_diagnostics();
    
    // Ensure cmake is available
    if (g_cmake_path.empty()) {
        add_diagnostic("Checking for CMake...", DiagnosticLevel::Info);
        g_cmake_path = forma::cmake::CMakeDownloader::ensure_cmake_available();
        
        if (g_cmake_path.empty()) {
            add_diagnostic("Failed to find or download CMake", DiagnosticLevel::Error);
            return;
        }
        
        if (g_cmake_path == "cmake") {
            add_diagnostic("Using system CMake", DiagnosticLevel::Info);
        } else {
            add_diagnostic("Downloaded CMake to: " + g_cmake_path, DiagnosticLevel::Info);
        }
    }
    
    // If a target triple is specified, ensure the cross-compiler is available
    if (!config_.target_triple.empty()) {
        add_diagnostic("Checking for " + config_.target_triple + " toolchain...", DiagnosticLevel::Info);
        
        // Check if we already have this compiler cached
        if (g_compiler_paths.find(config_.target_triple) == g_compiler_paths.end()) {
            std::string compiler_path = forma::toolchain::ToolchainManager::ensure_compiler_available(config_.target_triple);
            
            if (compiler_path.empty()) {
                add_diagnostic("Failed to find or download toolchain for " + config_.target_triple, DiagnosticLevel::Error);
                add_diagnostic("Supported targets: aarch64-linux-gnu, arm-linux-gnueabihf, x86_64-w64-mingw32, riscv64-linux-gnu", DiagnosticLevel::Info);
                return;
            }
            
            g_compiler_paths[config_.target_triple] = compiler_path;
            
            if (compiler_path.find("/") == std::string::npos) {
                add_diagnostic("Using system toolchain for " + config_.target_triple, DiagnosticLevel::Info);
            } else {
                add_diagnostic("Downloaded toolchain to: " + compiler_path, DiagnosticLevel::Info);
            }
        }
    }
    
    add_diagnostic("CMake generator initialized", DiagnosticLevel::Info);
}

void CMakeGenerator::compile(const char* source_file, const char* /*output_file*/) {
    if (!initialized_) {
        add_diagnostic("Generator not initialized", DiagnosticLevel::Error);
        return;
    }

    // Add source file to config
    add_source_file(source_file);
    
    // Generate CMakeLists.txt
    std::string cmake_path = config_.output_dir + "/CMakeLists.txt";
    generate_cmakelists(cmake_path);
    
    add_diagnostic(std::string("Generated CMakeLists.txt at ") + cmake_path, 
                   DiagnosticLevel::Info);
}

void CMakeGenerator::link(const char** /*object_files*/, int /*count*/, const char* output_binary) {
    if (!initialized_) {
        add_diagnostic("Generator not initialized", DiagnosticLevel::Error);
        return;
    }

    // For CMake, linking is handled in the CMakeLists.txt
    // We just regenerate with the final target name
    if (output_binary) {
        config_.target_name = std::filesystem::path(output_binary).stem().string();
    }
    
    std::string cmake_path = config_.output_dir + "/CMakeLists.txt";
    generate_cmakelists(cmake_path);
    
    add_diagnostic("Updated CMakeLists.txt with target: " + config_.target_name, 
                   DiagnosticLevel::Info);
}

void CMakeGenerator::clean() {
    clear_diagnostics();
    add_diagnostic("Clean operation requested", DiagnosticLevel::Info);
}

void CMakeGenerator::get_diagnostics(BuildDiagnostic** diagnostics, int* count) {
    if (diagnostics && count) {
        *diagnostics = diagnostics_.data();
        *count = static_cast<int>(diagnostics_.size());
    }
}

void CMakeGenerator::shutdown() {
    if (initialized_) {
        clear_diagnostics();
        initialized_ = false;
        context_ = nullptr;
    }
}

void CMakeGenerator::set_config(const CMakeGeneratorConfig& config) {
    config_ = config;
}

void CMakeGenerator::generate_cmakelists(const std::string& output_path) {
    // Ensure output directory exists
    std::filesystem::path dir = std::filesystem::path(output_path).parent_path();
    if (!dir.empty()) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream out(output_path);
    if (!out.is_open()) {
        add_diagnostic("Failed to create CMakeLists.txt at " + output_path, 
                       DiagnosticLevel::Error);
        return;
    }

    // Generate the CMakeLists.txt content
    out << generate_header();
    out << generate_project_declaration();
    out << generate_cxx_standard();
    out << "\n";
    out << generate_sources();
    out << "\n";
    out << generate_target();
    out << "\n";
    out << generate_includes();
    out << generate_link_libraries();
    out << generate_compile_options();
    out << "\n";
    out << generate_install_rules();

    out.close();
}

void CMakeGenerator::add_source_file(const std::string& file) {
    config_.source_files.push_back(file);
}

void CMakeGenerator::add_include_dir(const std::string& dir) {
    config_.include_dirs.push_back(dir);
}

void CMakeGenerator::add_library(const std::string& lib) {
    config_.link_libraries.push_back(lib);
}

std::string CMakeGenerator::generate_header() {
    std::stringstream ss;
    ss << "# Generated by Forma CMake Generator\n";
    ss << "# Do not edit manually - this file is auto-generated\n\n";
    ss << "cmake_minimum_required(VERSION " << config_.cmake_minimum_version << ")\n\n";
    return ss.str();
}

std::string CMakeGenerator::generate_project_declaration() {
    std::stringstream ss;
    ss << "project(" << config_.project_name << " LANGUAGES CXX)\n\n";
    return ss.str();
}

std::string CMakeGenerator::generate_cxx_standard() {
    std::stringstream ss;
    ss << "set(CMAKE_CXX_STANDARD " << config_.cxx_standard << ")\n";
    ss << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n";
    ss << "set(CMAKE_CXX_EXTENSIONS OFF)\n";
    return ss.str();
}

std::string CMakeGenerator::generate_sources() {
    if (config_.source_files.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "set(SOURCES\n";
    for (const auto& file : config_.source_files) {
        ss << "    " << file << "\n";
    }
    ss << ")\n";
    return ss.str();
}

std::string CMakeGenerator::generate_includes() {
    if (config_.include_dirs.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "target_include_directories(" << config_.target_name << " PRIVATE\n";
    for (const auto& dir : config_.include_dirs) {
        ss << "    " << dir << "\n";
    }
    ss << ")\n\n";
    return ss.str();
}

std::string CMakeGenerator::generate_target() {
    std::stringstream ss;
    ss << "add_executable(" << config_.target_name;
    if (!config_.source_files.empty()) {
        ss << " ${SOURCES}";
    }
    ss << ")\n";
    return ss.str();
}

std::string CMakeGenerator::generate_link_libraries() {
    if (config_.link_libraries.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "target_link_libraries(" << config_.target_name << " PRIVATE\n";
    for (const auto& lib : config_.link_libraries) {
        ss << "    " << lib << "\n";
    }
    ss << ")\n\n";
    return ss.str();
}

std::string CMakeGenerator::generate_compile_options() {
    if (config_.compile_options.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "target_compile_options(" << config_.target_name << " PRIVATE\n";
    for (const auto& opt : config_.compile_options) {
        ss << "    " << opt << "\n";
    }
    ss << ")\n\n";
    return ss.str();
}

std::string CMakeGenerator::generate_install_rules() {
    std::stringstream ss;
    ss << "install(TARGETS " << config_.target_name << "\n";
    ss << "    RUNTIME DESTINATION bin\n";
    ss << ")\n";
    return ss.str();
}

void CMakeGenerator::add_diagnostic(const std::string& message, DiagnosticLevel level) {
    BuildDiagnostic diag;
    diag.message = message.c_str();
    diag.level = level;
    diag.line = 0;
    diag.column = 0;
    diag.file = nullptr;
    diagnostics_.push_back(diag);
}

void CMakeGenerator::clear_diagnostics() {
    diagnostics_.clear();
}

bool CMakeGenerator::run_cmake_configure() {
    if (!initialized_ || g_cmake_path.empty()) {
        add_diagnostic("CMake not available", DiagnosticLevel::Error);
        return false;
    }
    
    // Create build directory
    std::filesystem::create_directories(config_.output_dir);
    
    // Run cmake configure
    std::string cmd = "cd \"" + config_.output_dir + "\" && \"" + g_cmake_path + "\" ";
    cmd += "-G \"" + config_.generator + "\" ";
    cmd += "-DCMAKE_BUILD_TYPE=" + config_.build_type + " ";
    
    // If cross-compiling, set the C/CXX compilers
    if (!config_.target_triple.empty() && g_compiler_paths.find(config_.target_triple) != g_compiler_paths.end()) {
        std::string compiler_path = g_compiler_paths[config_.target_triple];
        
        // Extract compiler directory
        std::string compiler_dir = compiler_path;
        size_t last_slash = compiler_dir.find_last_of("/");
        if (last_slash != std::string::npos) {
            compiler_dir = compiler_dir.substr(0, last_slash);
        }
        
        // Set compiler paths
        cmd += "-DCMAKE_C_COMPILER=\"" + compiler_path + "\" ";
        
        // Derive C++ compiler path from C compiler
        std::string cxx_compiler = compiler_path;
        if (cxx_compiler.find("gcc") != std::string::npos) {
            size_t pos = cxx_compiler.rfind("gcc");
            cxx_compiler.replace(pos, 3, "g++");
        } else if (cxx_compiler.find("clang") != std::string::npos) {
            size_t pos = cxx_compiler.rfind("clang");
            cxx_compiler.replace(pos, 5, "clang++");
        }
        cmd += "-DCMAKE_CXX_COMPILER=\"" + cxx_compiler + "\" ";
        
        add_diagnostic("Using cross-compiler: " + compiler_path, DiagnosticLevel::Info);
    }
    
    cmd += "..";
    
    add_diagnostic("Running: " + cmd, DiagnosticLevel::Info);
    
    int result = system(cmd.c_str());
    if (result != 0) {
        add_diagnostic("CMake configure failed", DiagnosticLevel::Error);
        return false;
    }
    
    add_diagnostic("CMake configure successful", DiagnosticLevel::Info);
    return true;
}

bool CMakeGenerator::run_cmake_build() {
    if (!initialized_ || g_cmake_path.empty()) {
        add_diagnostic("CMake not available", DiagnosticLevel::Error);
        return false;
    }
    
    // Run cmake build
    std::string cmd = "\"" + g_cmake_path + "\" --build \"" + config_.output_dir + "\"";
    
    add_diagnostic("Running: " + cmd, DiagnosticLevel::Info);
    
    int result = system(cmd.c_str());
    if (result != 0) {
        add_diagnostic("CMake build failed", DiagnosticLevel::Error);
        return false;
    }
    
    add_diagnostic("CMake build successful", DiagnosticLevel::Info);
    return true;
}

// VTable wrapper implementations
namespace cmake_vtable {
    void init(BuildContext* ctx) {
        if (!g_cmake_generator) {
            g_cmake_generator = std::make_unique<CMakeGenerator>();
        }
        g_cmake_generator->init(ctx);
    }

    void compile(const char* source_file, const char* output_file) {
        if (g_cmake_generator) {
            g_cmake_generator->compile(source_file, output_file);
        }
    }

    void link(const char** object_files, int count, const char* output_binary) {
        if (g_cmake_generator) {
            g_cmake_generator->link(object_files, count, output_binary);
        }
    }

    void clean() {
        if (g_cmake_generator) {
            g_cmake_generator->clean();
        }
    }

    void get_diagnostics(BuildDiagnostic** diagnostics, int* count) {
        if (g_cmake_generator) {
            g_cmake_generator->get_diagnostics(diagnostics, count);
        }
    }

    void shutdown() {
        if (g_cmake_generator) {
            g_cmake_generator->shutdown();
            g_cmake_generator.reset();
        }
    }
}
