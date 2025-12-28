#pragma once
#include "plugin.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <memory>

struct CMakeGeneratorConfig {
    std::string project_name;
    std::string cmake_minimum_version;
    std::string cxx_standard;
    std::string generator; // Ninja, Unix Makefiles, etc.
    std::string build_type; // Debug, Release, RelWithDebInfo
    std::vector<std::string> source_files;
    std::vector<std::string> include_dirs;
    std::vector<std::string> link_libraries;
    std::vector<std::string> compile_options;
    std::string output_dir;
    std::string target_name;
    std::string target_triple; // Target architecture/platform (e.g., "aarch64-linux-gnu")
};

class CMakeGenerator {
public:
    CMakeGenerator();
    ~CMakeGenerator();

    void init(BuildContext* ctx);
    void compile(const char* source_file, const char* output_file);
    void link(const char** object_files, int count, const char* output_binary);
    void clean();
    void get_diagnostics(BuildDiagnostic** diagnostics, int* count);
    void shutdown();

    // CMake-specific methods
    void set_config(const CMakeGeneratorConfig& config);
    void generate_cmakelists(const std::string& output_path);
    void add_source_file(const std::string& file);
    void add_include_dir(const std::string& dir);
    void add_library(const std::string& lib);
    
    // Run cmake configure and build
    bool run_cmake_configure();
    bool run_cmake_build();

private:
    BuildContext* context_;
    CMakeGeneratorConfig config_;
    std::vector<BuildDiagnostic> diagnostics_;
    bool initialized_;

    std::string generate_header();
    std::string generate_project_declaration();
    std::string generate_cxx_standard();
    std::string generate_sources();
    std::string generate_includes();
    std::string generate_target();
    std::string generate_link_libraries();
    std::string generate_compile_options();
    std::string generate_install_rules();
    
    void add_diagnostic(const std::string& message, DiagnosticLevel level);
    void clear_diagnostics();
};

// C-style vtable wrapper functions
namespace cmake_vtable {
    void init(BuildContext* ctx);
    void compile(const char* source_file, const char* output_file);
    void link(const char** object_files, int count, const char* output_binary);
    void clean();
    void get_diagnostics(BuildDiagnostic** diagnostics, int* count);
    void shutdown();
}

// Global instance for vtable callbacks
extern std::unique_ptr<CMakeGenerator> g_cmake_generator;

// VTable instance
extern BuildVTable cmake_build_vtable;
