// Forma Programming Language - Main Compiler Entry Point

#include "src/parser/ir.hpp"
#include "src/parser/semantic.hpp"
#include "src/core/assets.hpp"
#include "src/core/pipeline.hpp"
#include "src/plugin_loader.hpp"
#include "src/commands/init.hpp"
#include "src/commands/deploy.hpp"
#include "src/commands/build.hpp"
#include "src/commands/run.hpp"
#include "plugins/tracer/src/tracer_plugin.hpp"
#include "plugins/lvgl-renderer/src/lvgl_renderer_builtin.hpp"
#include "src/core/toml_io.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

struct CompilerOptions {
    std::string mode = "compile";
    std::string renderer;
    std::vector<std::string> plugins;
    std::vector<std::string> plugin_dirs;
    std::string project_path;
    std::string build_system;
    std::string target_triple;
    std::string target;  // Target platform: esp32, linux, windows, etc.
    std::string project_name;
    std::string plugin_type;  // Plugin type for init plugin
    std::string input_file;
    std::vector<std::string> deploy_systems;
    std::vector<std::string> architectures;
    bool verbose = false;
    bool debug = false;
    bool list_plugins = false;
    bool is_plugin = false;  // true for forma init plugin
    bool flash = false;      // Flash to device after build
    bool monitor = false;    // Start monitor after flash
};

int load_plugins(forma::IPluginLoader& plugin_loader, const std::vector<std::string>& plugin_names,
                 forma::tracer::TracerPlugin*& active_tracer) {
    auto& tracer = forma::tracer::get_tracer();
    
    for (const auto& plugin_name : plugin_names) {
        std::string error_msg;
        tracer.verbose(std::string("Loading plugin: ") + plugin_name);
        
        if (!plugin_loader.load_plugin_by_name(plugin_name, error_msg)) {
            tracer.error(error_msg);
            return 1;
        }
        
        // Plugin loaded successfully - metadata is available
        tracer.info(std::string("✓ Loaded plugin: ") + plugin_name);
        
        // TODO: Check if this is a tracer plugin and switch to it
        // For now, tracer plugins aren't supported yet
        (void)active_tracer; // Suppress unused warning
    }
    
    return 0;
}

int load_plugin_directories(forma::IPluginLoader& plugin_loader, const std::vector<std::string>& plugin_dirs) {
    auto& tracer = forma::tracer::get_tracer();
    
    for (const auto& dir_path : plugin_dirs) {
        tracer.verbose(std::string("Adding plugin search path: ") + dir_path);
        plugin_loader.add_plugin_search_path(dir_path);
    }
    
    return 0;
}

std::string read_source_file(const std::string& input_file, forma::tracer::TracerPlugin& tracer) {
    tracer.begin_stage("Reading source file");
    tracer.verbose(std::string("File: ") + input_file);
    
    std::ifstream file(input_file);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    if (source.empty()) {
        tracer.error("Input file is empty");
        std::exit(1);
    }
    
    tracer.stat("File size", source.size());
    tracer.end_stage();
    
    return source;
}

template<typename DocType>
int generate_code(const DocType& doc, const std::string& input_file, 
                  const std::string& renderer, forma::IPluginLoader& plugin_loader,
                  forma::tracer::TracerPlugin& tracer) {
    // Check renderer selection before code generation
    std::vector<std::string> available_renderers;
    for (const auto& plugin : plugin_loader.get_loaded_plugins()) {
        if (plugin->metadata && plugin->metadata->is_renderer()) {
            available_renderers.push_back(plugin->metadata->name);
        }
    }
    
    // If only the built-in lvgl renderer is available and no renderer specified, use it as default
    std::string target_renderer = renderer;
    if (renderer.empty() && available_renderers.size() == 1 && available_renderers[0] == "lvgl") {
        target_renderer = "lvgl";
        tracer.verbose("Auto-selected default renderer: lvgl");
    }
    // If plugins loaded but no --renderer specified, error (be explicit)
    else if (!available_renderers.empty() && renderer.empty()) {
        tracer.error("Renderer plugins loaded but no --renderer specified");
        tracer.info("Available plugin renderers:");
        for (const auto& name : available_renderers) {
            tracer.info(std::string("  - ") + name);
        }
        tracer.info("\nSpecify --renderer <name> to select one");
        return 1;
    }
    // No renderers available at all
    else if (available_renderers.empty()) {
        tracer.error("No renderer plugins available");
        return 1;
    }
    // Use specified renderer
    else if (!renderer.empty()) {
        target_renderer = renderer;
    }

    tracer.begin_stage("Code generation");
    
    std::string output_file = input_file;
    size_t dot_pos = output_file.find_last_of('.');
    if (dot_pos != std::string::npos) {
        output_file = output_file.substr(0, dot_pos);
    }
    
    // Find matching renderer plugin (exact name match)
    forma::LoadedPlugin* selected_plugin = nullptr;
    for (const auto& plugin : plugin_loader.get_loaded_plugins()) {
        if (plugin->metadata && 
            plugin->metadata->is_renderer() &&
            plugin->metadata->name == target_renderer) {
            selected_plugin = plugin.get();
            break;
        }
    }
    
    // Use plugin renderer if found
    if (selected_plugin) {
        tracer.verbose(std::string("Using plugin renderer: ") + selected_plugin->metadata->name);

        // Get output extension from metadata
        std::string extension = ".gen";  // default fallback
        if (!selected_plugin->metadata->output_extension.empty()) {
            extension = selected_plugin->metadata->output_extension;
            tracer.verbose(std::string("Using output extension from metadata: ") + extension);
        }
        output_file += extension;

        tracer.verbose(std::string("Output: ") + output_file);

        // Use adapter if available
        auto renderer_adapter = plugin_loader.get_renderer_adapter(selected_plugin->metadata->name);
        if (!renderer_adapter) {
            tracer.error("Plugin does not provide a render adapter");
            return 1;
        }

        // Use RealFileSystem for CLI compile
        forma::fs::RealFileSystem realfs;
        if (!renderer_adapter(&doc, input_file, output_file, realfs)) {
            tracer.error("Plugin rendering failed");
            return 1;
        }
        tracer.end_stage();
    } 
    // No renderer found - error
    else {
        tracer.error(std::string("Unknown renderer: ") + target_renderer);
        tracer.info("Available renderers:");
        for (const auto& name : available_renderers) {
            tracer.info(std::string("  - ") + name);
        }
        return 1;
    }

    tracer.info("\n✓ Compilation successful");
    tracer.info(std::string("  Output: ") + output_file);
    
    return 0;
}

int main(int argc, char* argv[]) {
    // Determine project/application version from forma.toml if present
    std::string app_version = "0.1.0";
    try {
        std::filesystem::path config_path = std::filesystem::current_path() / "forma.toml";
        if (std::filesystem::exists(config_path)) {
            forma::fs::RealFileSystem realfs;
            std::string content;
            if (forma::core::read_toml_file(realfs, config_path.string(), content)) {
                auto toml_doc_opt = forma::core::parse_toml_from_fs(realfs, config_path.string());
                if (toml_doc_opt) {
                    auto& toml_doc = *toml_doc_opt;
                    if (auto pkg = toml_doc.get_table("package")) {
                        if (auto ver = pkg->get_string("version")) {
                            app_version = std::string(*ver);
                        }
                    } else if (auto ver = toml_doc.root.get_string("version")) {
                        app_version = std::string(*ver);
                    }
                }
            }
        }
    } catch (...) {
        // Fall back to default version on any parse/IO error
    }

    CLI::App app{"Forma Programming Language"};
    app.set_version_flag("--version", app_version.c_str());
    app.footer("A QML-inspired programming language");

    CompilerOptions opts;
    
    // Global options
    app.add_flag("-v,--verbose", opts.verbose, "Enable verbose output");
    app.add_flag("--debug", opts.debug, "Enable debug output");
    app.add_option("--renderer", opts.renderer, "Renderer backend: js, sdl, lvgl, vulkan");
    app.add_option("--plugin", opts.plugins, "Load plugin by name (e.g., c-codegen, lvgl-renderer)")->expected(1, -1);
    app.add_option("--plugin-dir", opts.plugin_dirs, "Add directory to plugin search path")->expected(1, -1);
    app.add_flag("--list-plugins", opts.list_plugins, "List all loaded plugins");
    app.add_option("--project", opts.project_path, "Project directory");
    app.add_option("input_file", opts.input_file, "Input file");
    
    // Allow global options to be used with subcommands
    app.allow_extras();

    // Init command
    auto* init_cmd = app.add_subcommand("init", "Initialize a new Forma project");
    init_cmd->add_option("--name", opts.project_name, "Project name");
    init_cmd->add_option("--build", opts.build_system, "Build system: cmake, meson, bazel");
    init_cmd->add_option("--target", opts.target, "Target platform: esp32, esp32s3, linux, windows");
    init_cmd->add_option("--project", opts.project_path, "Project directory");
    init_cmd->callback([&opts]() { opts.mode = "init"; });

    // Init plugin subcommand
    auto* init_plugin_cmd = app.add_subcommand("init-plugin", "Initialize a new Forma plugin");
    init_plugin_cmd->add_option("--name", opts.project_name, "Plugin name");
    init_plugin_cmd->add_option("--type", opts.plugin_type, "Plugin type: renderer, lsp, build");
    init_plugin_cmd->add_option("--project", opts.project_path, "Project directory");
    init_plugin_cmd->callback([&opts]() { 
        opts.mode = "init"; 
        opts.is_plugin = true;
    });

    // Build command
    auto* build_cmd = app.add_subcommand("build", "Build project for target platform");
    build_cmd->add_option("--target", opts.target, "Target platform");
    build_cmd->add_option("--project", opts.project_path, "Project directory");
    build_cmd->add_flag("--flash", opts.flash, "Flash to device after build (embedded targets)");
    build_cmd->add_flag("--monitor", opts.monitor, "Start serial monitor after flash (embedded targets)");
    build_cmd->callback([&opts]() { opts.mode = "build"; });

    // Run command
    auto* run_cmd = app.add_subcommand("run", "Compile, build and run project");
    run_cmd->add_option("--target", opts.target, "Target platform");
    run_cmd->add_option("--project", opts.project_path, "Project directory");
    run_cmd->add_flag("--flash", opts.flash, "Flash to device after build (embedded targets)");
    run_cmd->add_flag("--monitor", opts.monitor, "Start serial monitor after flash (embedded targets)");
    run_cmd->callback([&opts]() { opts.mode = "run"; });

    // Deploy command
    auto* deploy_cmd = app.add_subcommand("deploy", "Build and package project for deployment");
    deploy_cmd->add_option("--deploy-system", opts.deploy_systems, "Deploy packaging system: deb, rpm, etc.")->expected(1, -1);
    deploy_cmd->add_option("--arch", opts.architectures, "Target architectures: amd64, arm64, armhf, etc.")->expected(1, -1);
    deploy_cmd->add_option("--project", opts.project_path, "Project directory");
    deploy_cmd->callback([&opts]() { opts.mode = "deploy"; });

    // Compile command (default)
    auto* compile_cmd = app.add_subcommand("compile", "Compile Forma source file");
    compile_cmd->add_option("--mode", opts.mode, "Execution mode: compile, lsp, repl")->default_val("compile");

    // Parse arguments
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // Handle debug flag
    if (opts.debug) {
        opts.verbose = true;
    }

    // Handle init command
    if (opts.mode == "init") {
        forma::commands::InitOptions init_opts;
        init_opts.project_name = opts.project_name.empty() ? (opts.is_plugin ? "myplugin" : "myapp") : opts.project_name;
        init_opts.is_plugin = opts.is_plugin;
        init_opts.plugin_type = opts.plugin_type.empty() ? "renderer" : opts.plugin_type;
        if (!opts.project_path.empty()) {
            init_opts.project_dir = opts.project_path;
        } else if (!opts.project_name.empty()) {
            init_opts.project_dir = opts.project_name;
        } else {
            init_opts.project_dir = opts.is_plugin ? init_opts.project_name : ".";
        }
        init_opts.verbose = opts.verbose;
        
        if (opts.is_plugin) {
            return forma::commands::run_plugin_init(init_opts);
        }
        
        init_opts.build_system = opts.build_system.empty() ? "cmake" : opts.build_system;
        init_opts.target_triple = opts.target_triple;
        init_opts.target = opts.target;
        init_opts.renderer = opts.renderer.empty() ? "lvgl" : opts.renderer;
        
        return forma::commands::run_init_command(init_opts);
    }
    
    // Handle build command
    if (opts.mode == "build") {
        forma::commands::BuildOptions build_opts;
        build_opts.project_dir = opts.project_path.empty() ? "." : opts.project_path;
        build_opts.target = opts.target;
        build_opts.verbose = opts.verbose;
        build_opts.flash = opts.flash;
        build_opts.monitor = opts.monitor;
        
        return forma::commands::run_build_command(build_opts);
    }
    
    // Handle run command
    if (opts.mode == "run") {
        forma::commands::RunOptions run_opts;
        run_opts.project_dir = opts.project_path.empty() ? "." : opts.project_path;
        run_opts.target = opts.target;
        run_opts.verbose = opts.verbose;
        run_opts.flash = opts.flash;
        run_opts.monitor = opts.monitor;
        
        return forma::commands::run_run_command(run_opts);
    }
    
    // Handle deploy command
    if (opts.mode == "deploy") {
        forma::commands::DeployOptions deploy_opts;
        deploy_opts.project_dir = opts.project_path.empty() ? "." : opts.project_path;
        deploy_opts.deploy_systems = opts.deploy_systems;
        deploy_opts.architectures = opts.architectures;
        deploy_opts.verbose = opts.verbose;

        return forma::commands::run_deploy_command(deploy_opts);
    }
    
    // Configure tracer
    auto& tracer = forma::tracer::get_tracer();
    if (opts.debug) {
        tracer.set_level(forma::tracer::TraceLevel::Debug);
    } else if (opts.verbose) {
        tracer.set_level(forma::tracer::TraceLevel::Verbose);
    } else {
        tracer.set_level(forma::tracer::TraceLevel::Normal);
    }

    // Load plugins
    auto plugin_loader = std::make_unique<forma::PluginLoader>();
    forma::tracer::TracerPlugin* active_tracer = &tracer;
    
    // Register built-in LVGL renderer plugin with inline metadata
    auto lvgl_metadata = std::make_unique<forma::PluginMetadata>();
    lvgl_metadata->name = "lvgl";
    lvgl_metadata->kind = "renderer";
    lvgl_metadata->api_version = "1.0.0";
    lvgl_metadata->runtime = "native";
    lvgl_metadata->provides = {"renderer:lvgl", "renderer:c", "widgets:basic", "widgets:lvgl", "animation", "events", "layouts"};
    lvgl_metadata->output_extension = ".c";
    lvgl_metadata->output_language = "c";
    
    plugin_loader->register_builtin_plugin(
        forma::lvgl::lvgl_builtin_render,
        nullptr,
        std::move(lvgl_metadata)
    );
    
    if (load_plugins(*plugin_loader, opts.plugins, active_tracer) != 0) {
        return 1;
    }
    
    if (load_plugin_directories(*plugin_loader, opts.plugin_dirs) != 0) {
        return 1;
    }
    
    if (opts.list_plugins) {
        plugin_loader->print_loaded_plugins();
        return 0;
    }

    // Validate input file
    if (opts.input_file.empty()) {
        active_tracer->error("No input file specified");
        std::cout << app.help() << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(opts.input_file)) {
        active_tracer->error(std::string("Input file not found: ") + opts.input_file);
        return 1;
    }

    // Print header
    active_tracer->info("Forma Compiler v0.1.0");
    active_tracer->info("=====================\n");
    active_tracer->verbose(std::string("Input: ") + opts.input_file);
    active_tracer->verbose(std::string("Mode: ") + opts.mode);
    if (!opts.renderer.empty()) {
        active_tracer->verbose(std::string("Renderer: ") + opts.renderer);
    }
    
    // Read and parse source
    auto source = read_source_file(opts.input_file, *active_tracer);
    
    active_tracer->begin_stage("Parsing");
    auto doc = forma::parse_document(source);
    active_tracer->stat("Types", doc.type_count);
    active_tracer->stat("Enums", doc.enum_count);
    active_tracer->stat("Events", doc.event_count);
    active_tracer->stat("Imports", doc.import_count);
    active_tracer->stat("Instances", doc.instances.count);
    active_tracer->end_stage();

    // Resolve imports
    forma::pipeline::resolve_imports(doc, opts.input_file, *active_tracer);

    // Type check
    if (forma::pipeline::run_semantic_analysis(doc, *active_tracer) != 0) {
        return 1;
    }

    // Collect assets
    forma::pipeline::collect_assets(doc, *active_tracer);

    // LSP mode stops here
    if (opts.mode == "lsp") {
        active_tracer->info("LSP mode - analysis complete");
        return 0;
    }

    // Generate code
    return generate_code(doc, opts.input_file, opts.renderer, *plugin_loader, *active_tracer);
}