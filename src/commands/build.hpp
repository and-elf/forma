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

ProjectConfig read_project_config(const std::string& project_dir, forma::tracer::TracerPlugin& tracer) {
    ProjectConfig config;
    
    std::filesystem::path project_path(project_dir);
    std::filesystem::path toml_path = project_path / "project.toml";
    
    // Try forma.toml if project.toml doesn't exist
    if (!std::filesystem::exists(toml_path)) {
        toml_path = project_path / "forma.toml";
    }
    
    if (!std::filesystem::exists(toml_path)) {
        tracer.error("No project.toml or forma.toml found in project directory");
        return config;
    }
    
    tracer.verbose(std::string("Reading project configuration: ") + toml_path.string());
    
    // Read TOML file
    std::ifstream file(toml_path);
    if (!file.is_open()) {
        tracer.error(std::string("Failed to open: ") + toml_path.string());
        return config;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string toml_content = buffer.str();
    
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
    auto src_dir = project_path / "src";
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
    
    // Read project configuration
    auto config = read_project_config(project_dir, tracer);
    
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
        
        forma::PluginLoader plugin_loader;
        
        // Load renderer plugin
        std::string error_msg;
        if (!plugin_loader.load_plugin_by_name(config.renderer, error_msg)) {
            tracer.error(std::string("Failed to load renderer plugin: ") + error_msg);
            return 1;
        }
        
        // Compile each source file
        for (const auto& source_file : config.source_files) {
            tracer.verbose(std::string("Compiling: ") + source_file);
            
            // Read source
            std::ifstream file(source_file);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string source = buffer.str();
            
            // Parse
            auto doc = forma::parse_document(source);
            
            // Run pipeline
            forma::pipeline::resolve_imports(doc, source_file, tracer);
            if (forma::pipeline::run_semantic_analysis(doc, tracer) != 0) {
                return 1;
            }
            forma::pipeline::collect_assets(doc, tracer);
            
            // Find renderer plugin
            forma::LoadedPlugin* renderer_plugin = nullptr;
            for (const auto& plugin : plugin_loader.get_loaded_plugins()) {
                if (plugin->metadata && plugin->metadata->is_renderer()) {
                    renderer_plugin = plugin.get();
                    break;
                }
            }
            
            if (!renderer_plugin || !renderer_plugin->functions.render) {
                tracer.error("Renderer plugin does not provide render function");
                return 1;
            }
            
            // Generate output path
            std::filesystem::path output_path = std::filesystem::path(source_file).replace_extension(
                renderer_plugin->metadata->output_extension
            );
            
            // Render
            if (!renderer_plugin->functions.render(&doc, source_file.c_str(), output_path.string().c_str())) {
                tracer.error(std::string("Code generation failed for: ") + source_file);
                return 1;
            }
            
            tracer.info(std::string("✓ Generated: ") + output_path.string());
        }
        
        tracer.end_stage();
    }
    
    // Step 2: Invoke build system plugin
    tracer.begin_stage("Building project");
    
    forma::PluginLoader build_plugin_loader;
    
    // Load build system plugin
    std::string error_msg;
    if (!build_plugin_loader.load_plugin_by_name(config.build_system, error_msg)) {
        tracer.error(std::string("Failed to load build plugin '") + config.build_system + "': " + error_msg);
        tracer.info("Build plugins handle compilation/linking after code generation");
        tracer.info("Available build plugins: cmake-generator, esp32-lvgl");
        return 1;
    }
    
    // Find the build plugin
    forma::LoadedPlugin* build_plugin = nullptr;
    for (const auto& plugin : build_plugin_loader.get_loaded_plugins()) {
        if (plugin->metadata && plugin->metadata->is_build()) {
            build_plugin = plugin.get();
            break;
        }
    }
    
    if (!build_plugin || !build_plugin->functions.build) {
        tracer.error("Build plugin does not provide build function");
        tracer.info("Build plugins must export a 'forma_build' function");
        return 1;
    }
    
    // Call the build function
    // forma_build(project_dir, config_path, verbose, flash, monitor)
    std::filesystem::path config_path = std::filesystem::path(project_dir) / "project.toml";
    if (!std::filesystem::exists(config_path)) {
        config_path = std::filesystem::path(project_dir) / "forma.toml";
    }
    
    int result = build_plugin->functions.build(
        project_dir.c_str(),
        config_path.string().c_str(),
        opts.verbose,
        opts.flash,
        opts.monitor
    );
    
    tracer.end_stage();
    
    if (result != 0) {
        tracer.error("Build failed");
        return result;
    }
    
    tracer.info("✓ Build complete");
    return 0;
}

} // namespace forma::commands
