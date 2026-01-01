#pragma once

#include "../toml/toml.hpp"
#include "../plugin_loader.hpp"
#include "../core/pipeline.hpp"
#include "../parser/ir.hpp"
#include "../../plugins/tracer/src/tracer_plugin.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace forma::commands {

struct BuildOptions {
    std::string project_dir;
    std::string target;  // esp32, linux, windows, etc.
    bool verbose = false;
    bool flash = false;   // Flash after build (for embedded targets)
    bool monitor = false; // Start monitor after flash
};

// Read project configuration and find source files
struct ProjectConfig {
    std::string build_system;  // cmake, esp-idf, meson, etc.
    std::string target;
    std::string renderer;
    std::vector<std::string> source_files;
    std::vector<std::string> plugins;
};

ProjectConfig read_project_config(const std::string& project_dir, forma::tracer::TracerPlugin& tracer, forma::fs::IFileSystem& fs) {
    ProjectConfig config;
    
    std::string toml_path = project_dir + "/project.toml";
    
    // Try forma.toml if project.toml doesn't exist
    if (!fs.exists(toml_path)) {
        toml_path = project_dir + "/forma.toml";
    }

    if (!fs.exists(toml_path)) {
        tracer.error("No project.toml or forma.toml found in project directory");
        return config;
    }
    
    tracer.verbose(std::string("Reading project configuration: ") + toml_path.string());
    
    // Read TOML file
    std::string toml_content = fs.read_file(toml_path);
    
    // Parse TOML
    auto doc = forma::toml::parse(toml_content);
    
    // Get [build] table
    if (auto* build_table = doc.get_table("build")) {
        if (auto val = build_table->get_string("system")) {
            config.build_system = std::string(*val);
        }
        if (auto val = build_table->get_string("target")) {
            config.target = std::string(*val);
        }
        if (auto val = build_table->get_string("renderer")) {
            config.renderer = std::string(*val);
        }
    }
    
    // Find all .fml files in src/ directory
    // Source file discovery: naive approach using filesystem for now
    std::filesystem::path src_dir(project_dir);
    src_dir /= "src";
    if (std::filesystem::exists(src_dir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(src_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".fml") {
                config.source_files.push_back(entry.path().string());
            }
        }
    }
    
    return config;
}

int run_build_command(const BuildOptions& opts) {
    auto& tracer = forma::tracer::get_tracer();
    
    if (opts.verbose) {
        tracer.set_level(forma::tracer::TraceLevel::Verbose);
    }
    
    tracer.info("Forma Build Command");
    tracer.info("===================\n");
    
    std::string project_dir = opts.project_dir.empty() ? "." : opts.project_dir;
    forma::fs::RealFileSystem realfs;
    
    // Read project configuration
    auto config = read_project_config(project_dir, tracer, realfs);
    
    if (config.build_system.empty()) {
        tracer.error("No build system specified in project configuration");
        tracer.info("Add [build] section to project.toml with system = \"cmake\" or \"esp-idf\"");
        return 1;
    }
    
    // Use CLI target if specified, otherwise use project config
    std::string target = opts.target.empty() ? config.target : opts.target;
    
    if (target.empty()) {
        target = "native";
        tracer.verbose("Using default target: native");
    } else {
        tracer.info(std::string("Target: ") + target);
    }
    
    tracer.info(std::string("Build system: ") + config.build_system);
    tracer.info(std::string("Source files: ") + std::to_string(config.source_files.size()));
    
    // Step 1: Compile all .fml source files using the renderer
    if (!config.source_files.empty() && !config.renderer.empty()) {
        tracer.begin_stage("Generating code");
        
        forma::PluginLoader plugin_loader_impl;
        forma::IPluginLoader& plugin_loader = plugin_loader_impl;

        // Load renderer plugin
        std::string error_msg;
        if (!plugin_loader.load_plugin_by_name(config.renderer, error_msg)) {
            tracer.error(std::string("Failed to load renderer plugin: ") + error_msg);
            return 1;
        }

        // Compile each source file
        for (const auto& source_file : config.source_files) {
            tracer.verbose(std::string("Compiling: ") + source_file);

            // Read source using realfs for now
            std::string source = realfs.read_file(source_file);

            // Parse
            auto doc = forma::parse_document(source);

            // Run pipeline
            forma::pipeline::resolve_imports(doc, source_file, tracer);
            if (forma::pipeline::run_semantic_analysis(doc, tracer) != 0) {
                return 1;
            }
            forma::pipeline::collect_assets(doc, tracer);

            // Get renderer adapter
            auto renderer_adapter = plugin_loader.get_renderer_adapter(config.renderer);
            if (!renderer_adapter) {
                tracer.error("Renderer plugin does not provide render adapter");
                return 1;
            }

            // Generate output path (use metadata)
            auto plugin = plugin_loader.find_plugin(config.renderer);
            std::string out_ext = plugin->metadata->output_extension;
            std::string output_path = std::filesystem::path(source_file).replace_extension(out_ext).string();

            // Call adapter (it will use IFileSystem to write output back)
            if (!renderer_adapter(&doc, source_file, output_path, realfs)) {
                tracer.error(std::string("Code generation failed for: ") + source_file);
                return 1;
            }

            tracer.info(std::string("✓ Generated: ") + output_path);
        }
        
        tracer.end_stage();
    }
    
    // Step 2: Invoke build system plugin
    tracer.begin_stage("Building project");
    
    forma::PluginLoader build_plugin_loader_impl;
    forma::IPluginLoader& build_plugin_loader = build_plugin_loader_impl;

    // Load build system plugin
    std::string error_msg;
    if (!build_plugin_loader.load_plugin_by_name(config.build_system, error_msg)) {
        tracer.error(std::string("Failed to load build plugin '") + config.build_system + "': " + error_msg);
        tracer.info("Build plugins handle compilation/linking after code generation");
        tracer.info("Available build plugins: cmake-generator, esp32-lvgl");
        return 1;
    }

    // Get build adapter
    auto build_adapter = build_plugin_loader.get_build_adapter(config.build_system);
    if (!build_adapter) {
        tracer.error("Build plugin does not provide build adapter");
        tracer.info("Build plugins must export a 'forma_build' function or adapter");
        return 1;
    }

    // Determine config path within filesystem
    std::string config_path = project_dir + "/project.toml";
    if (!realfs.exists(config_path)) config_path = project_dir + "/forma.toml";

    // Call build adapter (it may create a temp project on disk and run plugin)
    int result = build_adapter(project_dir, config_path, realfs, opts.verbose, opts.flash, opts.monitor);
    
    tracer.end_stage();
    
    if (result != 0) {
        tracer.error("Build failed");
        return result;
    }
    
    tracer.info("✓ Build complete");
    return 0;
}

} // namespace forma::commands
