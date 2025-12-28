// Forma Programming Language - Main Compiler Entry Point

#include "src/parser/ir.hpp"
#include "src/parser/semantic.hpp"
#include "src/core/assets.hpp"
#include "src/plugin_loader.hpp"
#include "src/commands/init.hpp"
#include "plugins/tracer/src/tracer_plugin.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

// Built-in LVGL renderer (fallback if plugin not loaded)
#include "plugins/lvgl-renderer/src/lvgl_renderer.hpp"

struct CompilerOptions {
    std::string mode = "compile";
    std::string renderer;
    std::vector<std::string> plugins;
    std::string project_path;
    std::string build_system;
    std::string target_triple;
    std::string project_name;
    std::string input_file;
    bool verbose = false;
    bool debug = false;
    bool list_plugins = false;
};

void print_usage(const char* program_name) {
    std::cout << "Forma Programming Language v0.1.0\n\n";
    std::cout << "Usage: " << program_name << " [command] [options] <input-file>\n\n";
    std::cout << "Commands:\n";
    std::cout << "  init                 Initialize a new Forma project\n\n";
    std::cout << "Options:\n";
    std::cout << "  --mode <mode>        Execution mode: compile, lsp, repl, init\n";
    std::cout << "  --renderer <name>    Renderer backend: js, sdl, lvgl, vulkan\n";
    std::cout << "  --plugin <path>      Load plugin from shared library (.so)\n";
    std::cout << "  --list-plugins       List all loaded plugins\n";
    std::cout << "  --project <path>     Project directory\n";
    std::cout << "  --build <system>     Build system: cmake, meson, bazel\n";
    std::cout << "  --target <triple>    Target architecture (e.g., aarch64-linux-gnu)\n";
    std::cout << "  --name <name>        Project name (for init command)\n";
    std::cout << "  -v, --verbose        Enable verbose output\n";
    std::cout << "  --debug              Enable debug output\n";
    std::cout << "  --help               Show this help message\n";
    std::cout << "  --version            Show version information\n";
}

void print_version() {
    std::cout << "Forma v0.1.0\n";
    std::cout << "A QML-inspired programming language\n";
}

CompilerOptions parse_arguments(int argc, char* argv[]) {
    CompilerOptions opts;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "--version") {
            print_version();
            std::exit(0);
        } else if (arg == "init") {
            opts.mode = "init";
        } else if (arg == "--list-plugins") {
            opts.list_plugins = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--debug") {
            opts.debug = true;
            opts.verbose = true;
        } else if (arg == "--mode" && i + 1 < argc) {
            opts.mode = argv[++i];
        } else if (arg == "--renderer" && i + 1 < argc) {
            opts.renderer = argv[++i];
        } else if (arg == "--plugin" && i + 1 < argc) {
            opts.plugins.push_back(argv[++i]);
        } else if (arg == "--project" && i + 1 < argc) {
            opts.project_path = argv[++i];
        } else if (arg == "--build" && i + 1 < argc) {
            opts.build_system = argv[++i];
        } else if (arg == "--target" && i + 1 < argc) {
            opts.target_triple = argv[++i];
        } else if (arg == "--name" && i + 1 < argc) {
            opts.project_name = argv[++i];
        } else if (arg[0] != '-') {
            opts.input_file = arg;
        }
    }
    
    return opts;
}

int load_plugins(forma::PluginLoader& plugin_loader, const std::vector<std::string>& plugin_paths,
                 forma::tracer::TracerPlugin*& active_tracer) {
    auto& tracer = forma::tracer::get_tracer();
    
    for (const auto& plugin_path : plugin_paths) {
        std::string error_msg;
        tracer.verbose(std::string("Loading plugin: ") + plugin_path);
        
        if (!plugin_loader.load_plugin(plugin_path, error_msg)) {
            tracer.error(error_msg);
            return 1;
        }
        
        auto* loaded = plugin_loader.find_plugin("");
        if (loaded && loaded->descriptor) {
            tracer.info(std::string("✓ Loaded plugin: ") + loaded->descriptor->name + 
                       " v" + loaded->descriptor->version);
            
            // TODO: Check if this is a tracer plugin and switch to it
            // For now, tracer plugins aren't supported yet
            (void)active_tracer; // Suppress unused warning
        }
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
void resolve_imports(DocType& doc, const std::string& input_file, forma::tracer::TracerPlugin& tracer) {
    if (doc.import_count == 0) {
        return;
    }
    
    tracer.begin_stage("Resolving imports");
    
    std::vector<std::string> loaded_files;
    loaded_files.push_back(std::filesystem::absolute(input_file).string());
    
    std::vector<DocType> imported_docs;
    std::vector<DocType> to_process;
    to_process.push_back(doc);
    
    auto base_dir = std::filesystem::path(input_file).parent_path();
    
    while (!to_process.empty()) {
        auto current_doc = to_process.back();
        to_process.pop_back();
        
        for (size_t i = 0; i < current_doc.import_count; ++i) {
            const auto& import_decl = current_doc.imports[i];
            std::string import_path(import_decl.module_path.data(), import_decl.module_path.size());
            
            // Convert dot notation to file path: components.Button -> components/Button.fml
            std::string file_path = import_path;
            std::replace(file_path.begin(), file_path.end(), '.', '/');
            file_path += ".fml";
            
            auto full_path = (base_dir / file_path).lexically_normal();
            auto canonical_path = std::filesystem::absolute(full_path).string();
            
            // Skip if already loaded
            if (std::find(loaded_files.begin(), loaded_files.end(), canonical_path) != loaded_files.end()) {
                tracer.verbose(std::string("  Already loaded: ") + import_path);
                continue;
            }
            
            if (!std::filesystem::exists(full_path)) {
                tracer.error(std::string("Import not found: ") + import_path + " (" + file_path + ")");
                std::exit(1);
            }
            
            tracer.verbose(std::string("  Loading: ") + import_path);
            
            std::ifstream import_file(full_path);
            std::stringstream import_buffer;
            import_buffer << import_file.rdbuf();
            std::string import_source = import_buffer.str();
            
            auto imported = forma::parse_document(import_source);
            imported_docs.push_back(imported);
            to_process.push_back(imported);
            loaded_files.push_back(canonical_path);
            
            tracer.verbose(std::string("    Types: ") + std::to_string(imported.type_count) + 
                         ", Enums: " + std::to_string(imported.enum_count));
        }
    }
    
    // Merge all imported documents into main document
    for (const auto& imported : imported_docs) {
        for (size_t i = 0; i < imported.type_count && doc.type_count < doc.types.size(); ++i) {
            doc.types[doc.type_count++] = imported.types[i];
        }
        for (size_t i = 0; i < imported.enum_count && doc.enum_count < doc.enums.size(); ++i) {
            doc.enums[doc.enum_count++] = imported.enums[i];
        }
        for (size_t i = 0; i < imported.event_count && doc.event_count < doc.events.size(); ++i) {
            doc.events[doc.event_count++] = imported.events[i];
        }
    }
    
    tracer.stat("Total files loaded", loaded_files.size());
    tracer.stat("Total types", doc.type_count);
    tracer.stat("Total enums", doc.enum_count);
    tracer.end_stage();
}

template<typename DocType>
int run_semantic_analysis(DocType& doc, forma::tracer::TracerPlugin& tracer) {
    tracer.begin_stage("Type checking");
    auto diagnostics = forma::analyze_document(doc);
    
    if (diagnostics.count > 0) {
        tracer.warning(std::string("Found ") + std::to_string(diagnostics.count) + " diagnostic(s)");
        
        for (size_t i = 0; i < diagnostics.count; ++i) {
            const auto& diag = diagnostics.diagnostics[i];
            std::string msg = std::string(diag.message.data(), diag.message.size()) + 
                            " (" + std::string(diag.code.data(), diag.code.size()) + ")";
            
            if (diag.severity == forma::DiagnosticSeverity::Error) {
                tracer.error(msg);
            } else {
                tracer.warning(msg);
            }
        }
        
        // Count errors
        size_t error_count = 0;
        for (size_t i = 0; i < diagnostics.count; ++i) {
            if (diagnostics.diagnostics[i].severity == forma::DiagnosticSeverity::Error) {
                error_count++;
            }
        }
        
        if (error_count > 0) {
            tracer.end_stage();
            tracer.error(std::string("Compilation failed with ") + std::to_string(error_count) + " error(s)");
            return 1;
        }
    }
    
    tracer.end_stage();
    return 0;
}

template<typename DocType>
void collect_assets(DocType& doc, forma::tracer::TracerPlugin& tracer) {
    tracer.begin_stage("Collecting assets");
    auto bundler = forma::collect_assets(doc);
    tracer.stat("Assets found", bundler.asset_count);
    
    for (size_t i = 0; i < bundler.asset_count; ++i) {
        const auto& asset = bundler.assets[i];
        tracer.verbose(std::string("  ") + std::string(asset.uri.data(), asset.uri.size()));
    }
    
    // Copy assets to document
    for (size_t i = 0; i < bundler.asset_count && i < doc.assets.size(); ++i) {
        doc.assets[i] = bundler.assets[i];
    }
    doc.asset_count = bundler.asset_count;
    tracer.end_stage();
}

template<typename DocType>
int generate_code(const DocType& doc, const std::string& input_file, 
                  const std::string& renderer, forma::PluginLoader& plugin_loader,
                  forma::tracer::TracerPlugin& tracer) {
    // Check renderer selection before code generation
    std::vector<std::string> available_renderers;
    for (const auto* plugin : plugin_loader.get_loaded_plugins()) {
        if (plugin->descriptor && plugin->descriptor->capabilities.supports_renderer) {
            available_renderers.push_back(plugin->descriptor->name);
        }
    }
    
    // If plugins loaded but no --renderer specified, error (be explicit)
    if (!available_renderers.empty() && renderer.empty()) {
        tracer.error("Renderer plugins loaded but no --renderer specified");
        tracer.info("Available plugin renderers:");
        for (const auto& name : available_renderers) {
            tracer.info(std::string("  - ") + name);
        }
        tracer.info("Built-in renderer: lvgl");
        tracer.info("\nSpecify --renderer <name> to select one");
        return 1;
    }

    tracer.begin_stage("Code generation");
    
    std::string output_file = input_file;
    size_t dot_pos = output_file.find_last_of('.');
    if (dot_pos != std::string::npos) {
        output_file = output_file.substr(0, dot_pos);
    }
    
    // Default to lvgl if no renderer specified (and no plugins)
    std::string target_renderer = renderer.empty() ? "lvgl" : renderer;
    
    // Find matching renderer plugin (exact name match)
    forma::LoadedPlugin* selected_plugin = nullptr;
    for (const auto* plugin : plugin_loader.get_loaded_plugins()) {
        if (plugin->descriptor && 
            plugin->descriptor->capabilities.supports_renderer &&
            plugin->descriptor->name == target_renderer) {
            selected_plugin = const_cast<forma::LoadedPlugin*>(plugin);
            break;
        }
    }
    
    // Use plugin renderer if found
    if (selected_plugin) {
        tracer.verbose(std::string("Using plugin renderer: ") + selected_plugin->descriptor->name);
        
        // Determine output extension based on renderer
        std::string extension = ".gen";
        if (target_renderer == "lvgl" || target_renderer == "lvgl-renderer") {
            extension = ".c";
        }
        output_file += extension;
        
        tracer.verbose(std::string("Output: ") + output_file);
        
        // Call plugin's render callback
        if (selected_plugin->descriptor->capabilities.render) {
            bool success = selected_plugin->descriptor->capabilities.render(
                &doc, input_file.c_str(), output_file.c_str()
            );
            
            if (!success) {
                tracer.error("Plugin rendering failed");
                return 1;
            }
            
            tracer.end_stage();
        } else {
            tracer.error("Plugin does not provide a render callback");
            return 1;
        }
    } 
    // Use built-in LVGL renderer as fallback
    else if (target_renderer == "lvgl") {
        output_file += ".c";
        tracer.verbose("Renderer: LVGL (built-in)");
        tracer.verbose(std::string("Output: ") + output_file);
        
        tracer.debug("Initializing LVGL renderer...");
        forma::lvgl::LVGLRenderer<65536> lvgl_renderer;
        
        tracer.debug("Generating code...");
        lvgl_renderer.generate(doc);
        
        tracer.debug("Writing output file...");
        std::ofstream out(output_file);
        out << lvgl_renderer.get_output();
        out.close();
        
        tracer.stat("Generated bytes", lvgl_renderer.get_output().size());
        tracer.end_stage();
    } 
    // No renderer found - error
    else {
        tracer.error(std::string("Unknown renderer: ") + target_renderer);
        tracer.info("Available built-in renderers: lvgl");
        
        // List available plugin renderers
        std::vector<std::string> plugin_renderers;
        for (const auto* plugin : plugin_loader.get_loaded_plugins()) {
            if (plugin->descriptor && plugin->descriptor->capabilities.supports_renderer) {
                plugin_renderers.push_back(plugin->descriptor->name);
            }
        }
        
        if (!plugin_renderers.empty()) {
            tracer.info("Loaded plugin renderers:");
            for (const auto& name : plugin_renderers) {
                tracer.info(std::string("  - ") + name);
            }
        } else {
            tracer.info("No plugin renderers loaded. Use --plugin to load one.");
        }
        return 1;
    }

    tracer.info("\n✓ Compilation successful");
    tracer.info(std::string("  Output: ") + output_file);
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse command line arguments
    auto opts = parse_arguments(argc, argv);
    
    // Handle init command early (before loading plugins or files)
    if (opts.mode == "init") {
        forma::commands::InitOptions init_opts;
        init_opts.project_name = opts.project_name.empty() ? "myapp" : opts.project_name;
        init_opts.build_system = opts.build_system.empty() ? "cmake" : opts.build_system;
        init_opts.target_triple = opts.target_triple;
        init_opts.renderer = opts.renderer.empty() ? "lvgl" : opts.renderer;
        init_opts.project_dir = opts.project_path.empty() ? "." : opts.project_path;
        init_opts.verbose = opts.verbose;
        
        return forma::commands::run_init_command(init_opts);
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
    forma::PluginLoader plugin_loader;
    forma::tracer::TracerPlugin* active_tracer = &tracer;
    
    if (load_plugins(plugin_loader, opts.plugins, active_tracer) != 0) {
        return 1;
    }
    
    if (opts.list_plugins) {
        plugin_loader.print_loaded_plugins();
        return 0;
    }

    // Validate input file
    if (opts.input_file.empty()) {
        active_tracer->error("No input file specified");
        print_usage(argv[0]);
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
    resolve_imports(doc, opts.input_file, *active_tracer);

    // Type check
    if (run_semantic_analysis(doc, *active_tracer) != 0) {
        return 1;
    }

    // Collect assets
    collect_assets(doc, *active_tracer);

    // LSP mode stops here
    if (opts.mode == "lsp") {
        active_tracer->info("LSP mode - analysis complete");
        return 0;
    }

    // Generate code
    return generate_code(doc, opts.input_file, opts.renderer, plugin_loader, *active_tracer);
}
