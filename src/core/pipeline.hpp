#pragma once

#include "../parser/ir.hpp"
#include "../parser/semantic.hpp"
#include "assets.hpp"
#include "../../plugins/tracer/src/tracer_plugin.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace forma::pipeline {

// ============================================================================
// Compilation Pipeline - Core compilation stages
// ============================================================================

// Resolve imports and load imported modules
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
    
    tracer.stat("Total files loaded", loaded_files.size());
    tracer.end_stage();
}

// Run semantic analysis and type checking
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

// Collect and process assets from the document
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

} // namespace forma::pipeline
