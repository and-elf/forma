#pragma once

#include <parser/ir.hpp>
#include <parser/semantic.hpp>
// Note: core/pipeline.hpp provides forma::pipeline::resolve_imports,
// forma::pipeline::run_semantic_analysis, and forma::pipeline::collect_assets
// for full compilation. LSP uses direct APIs for faster interactive feedback.
#include <string_view>
#include <string>
#include <array>
#include <memory>
#include <iostream>

namespace forma::lsp {

// ============================================================================
// Simple LSP Protocol Types (No full JSON parsing needed)
// ============================================================================

struct Position {
    int line = 0;      // 0-based
    int character = 0; // 0-based (UTF-16 code units)
    
    constexpr Position() = default;
    constexpr Position(int l, int c) : line(l), character(c) {}
};

struct Range {
    Position start;
    Position end;
    
    constexpr Range() = default;
    constexpr Range(Position s, Position e) : start(s), end(e) {}
    constexpr Range(int sl, int sc, int el, int ec) 
        : start(sl, sc), end(el, ec) {}
};

using DocumentUri = std::string_view;

struct Location {
    DocumentUri uri;
    Range range;
    
    constexpr Location() = default;
    constexpr Location(DocumentUri u, Range r) : uri(u), range(r) {}
};

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

struct Diagnostic {
    Range range;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string_view code;
    std::string_view message;
    
    constexpr Diagnostic() = default;
    constexpr Diagnostic(Range r, DiagnosticSeverity s, std::string_view c, std::string_view m)
        : range(r), severity(s), code(c), message(m) {}
};

// ============================================================================
// LSP Server Capabilities
// ============================================================================

struct TextDocumentSyncOptions {
    bool open_close = true;
    int change = 1; // 1 = Full sync
};

struct ServerCapabilities {
    TextDocumentSyncOptions text_document_sync;
    bool diagnostic_provider = true;
    
    constexpr ServerCapabilities() = default;
};

struct InitializeResult {
    ServerCapabilities capabilities;
    std::string_view server_name = "forma-lsp";
    std::string_view server_version = "0.1.0";
    
    constexpr InitializeResult() = default;
};

// ============================================================================
// Text Document Types
// ============================================================================

struct TextDocumentItem {
    DocumentUri uri;
    std::string_view language_id;
    int version = 0;
    std::string_view text;
    
    constexpr TextDocumentItem() = default;
};

struct TextDocumentIdentifier {
    DocumentUri uri;
    
    constexpr TextDocumentIdentifier() = default;
    constexpr TextDocumentIdentifier(DocumentUri u) : uri(u) {}
};

struct VersionedTextDocumentIdentifier {
    DocumentUri uri;
    int version = 0;
    
    constexpr VersionedTextDocumentIdentifier() = default;
};

// ============================================================================
// LSP Document Management
// ============================================================================

template<size_t MaxDocs = 16>
struct LSPDocumentManager {
    struct Document {
        std::string uri;  // Owned storage for URI
        std::string_view text;
        int version = 0;
        bool active = false;
        
        // Diagnostics for this document
        std::array<Diagnostic, 32> diagnostics;
        size_t diagnostic_count = 0;
        
        // Cached parsed document (owned storage)
        std::string cached_source;  // Store the source we last parsed
        std::unique_ptr<forma::Document<32, 16, 16, 32, 64, 64>> cached_ast; // Cached AST on heap
        bool cache_valid = false;    // Is the cache valid?
    };
    
    std::array<Document, MaxDocs> documents;
    size_t document_count = 0;
    bool initialized = false;
    
    LSPDocumentManager() = default;
    
    InitializeResult initialize(int process_id, DocumentUri root_uri) {
        (void)process_id;  // unused
        (void)root_uri;    // unused
        initialized = true;
        return InitializeResult{};
    }
    
    Document* find_document(DocumentUri uri) {
        std::cerr << "  Debug: find_document called with uri='" << uri << "', document_count=" << document_count << std::endl;
        for (size_t i = 0; i < document_count; ++i) {
            std::cerr << "  Debug: Comparing with doc[" << i << "].uri='" << documents[i].uri << "', active=" << documents[i].active << std::endl;
            if (documents[i].uri == uri && documents[i].active) {
                std::cerr << "  Debug: Found match at index " << i << std::endl;
                return &documents[i];
            }
        }
        std::cerr << "  Debug: No match found" << std::endl;
        return nullptr;
    }
    
    const Document* find_document(DocumentUri uri) const {
        for (size_t i = 0; i < document_count; ++i) {
            if (documents[i].uri == uri && documents[i].active) {
                return &documents[i];
            }
        }
        return nullptr;
    }
    
    void did_open(const TextDocumentItem& item) {
        // Find or create document slot
        Document* doc = find_document(item.uri);
        if (!doc && document_count < MaxDocs) {
            doc = &documents[document_count++];
        }
        
        if (doc) {
            doc->uri = std::string(item.uri.data(), item.uri.size());  // Store as owned string
            doc->version = item.version;
            doc->active = true;
            doc->diagnostic_count = 0;
            doc->cache_valid = false; // Invalidate cache
            
            // Store the text in cached_source (owned storage)
            // and point text view to it
            doc->cached_source = std::string(item.text.data(), item.text.size());
            doc->text = doc->cached_source;
            
            // Run diagnostics
            analyze_document(*doc);
        }
    }
    
    void did_change(const VersionedTextDocumentIdentifier& id, std::string_view new_text) {
        Document* doc = find_document(id.uri);
        if (doc) {
            doc->version = id.version;
            doc->diagnostic_count = 0;
            doc->cache_valid = false; // Invalidate cache on change
            
            // Store the text and update the view
            doc->cached_source = std::string(new_text.data(), new_text.size());
            doc->text = doc->cached_source;
            
            // Run diagnostics
            analyze_document(*doc);
        }
    }
    
    void did_close(const TextDocumentIdentifier& id) {
        Document* doc = find_document(id.uri);
        if (doc) {
            doc->active = false;
            doc->diagnostic_count = 0;
        }
    }
    
    // Find definition at position (for go-to-definition)
    bool find_definition(DocumentUri uri, Position pos, Location& out_location) {
        Document* doc = find_document(uri);
        if (!doc || !doc->cache_valid || !doc->cached_ast) {
            std::cerr << "  Debug: Document not found or cache invalid" << std::endl;
            return false;
        }
        
        // Extract the identifier at the cursor position from source text
        std::string_view source = doc->cached_source;
        std::string_view identifier = extract_identifier_at_position(source, pos);
        
        std::cerr << "  Debug: Extracted identifier: '" << identifier << "'" << std::endl;
        
        if (identifier.empty()) {
            std::cerr << "  Debug: Identifier is empty" << std::endl;
            return false;
        }
        
        // First, search the symbol table for this identifier
        const auto& symbols = doc->cached_ast->symbols;
        std::cerr << "  Debug: Searching " << symbols.count << " symbols" << std::endl;
        for (size_t i = 0; i < symbols.count; ++i) {
            const auto& sym = symbols.symbols[i];
            
            if (sym.name == identifier) {
                // Convert offset to line/column if needed
                size_t line, col;
                if (sym.location.line == 0 && sym.location.column == 0 && sym.location.offset > 0) {
                    // Location stored as offset, convert it
                    auto [l, c] = offset_to_position(source, sym.location.offset);
                    line = l;
                    col = c;
                    std::cerr << "  Debug: Found in symbol table at offset " << sym.location.offset 
                             << " = line " << line << ", col " << col << std::endl;
                } else {
                    // Location already has line/column
                    line = sym.location.line;
                    col = sym.location.column;
                    std::cerr << "  Debug: Found in symbol table at line " << line << std::endl;
                }
                
                // Found the definition in symbol table
                out_location.uri = uri;
                out_location.range.start.line = static_cast<int>(line);
                out_location.range.start.character = static_cast<int>(col);
                out_location.range.end.line = static_cast<int>(line);
                out_location.range.end.character = static_cast<int>(col + sym.name.size());
                return true;
            }
        }
        
        // If not in symbol table, search type declarations directly
        std::cerr << "  Debug: Not in symbol table, searching " << doc->cached_ast->type_count << " type declarations" << std::endl;
        for (size_t i = 0; i < doc->cached_ast->type_count; ++i) {
            const auto& type = doc->cached_ast->types[i];
            std::cerr << "  Debug: Checking type '" << type.name << "'" << std::endl;
            if (type.name == identifier) {
                // Found type declaration - search for it in source
                size_t type_pos = find_in_source(source, identifier);
                std::cerr << "  Debug: Found type, searching in source, offset: " << type_pos << std::endl;
                if (type_pos != std::string_view::npos) {
                    auto [line, col] = offset_to_position(source, type_pos);
                    std::cerr << "  Debug: Converted to line " << line << ", col " << col << std::endl;
                    out_location.uri = uri;
                    out_location.range.start.line = static_cast<int>(line);
                    out_location.range.start.character = static_cast<int>(col);
                    out_location.range.end.line = static_cast<int>(line);
                    out_location.range.end.character = static_cast<int>(col + identifier.size());
                    return true;
                }
            }
        }
        
        // Search enum declarations
        for (size_t i = 0; i < doc->cached_ast->enum_count; ++i) {
            const auto& enum_decl = doc->cached_ast->enums[i];
            if (enum_decl.name == identifier) {
                size_t enum_pos = find_in_source(source, identifier);
                if (enum_pos != std::string_view::npos) {
                    auto [line, col] = offset_to_position(source, enum_pos);
                    out_location.uri = uri;
                    out_location.range.start.line = static_cast<int>(line);
                    out_location.range.start.character = static_cast<int>(col);
                    out_location.range.end.line = static_cast<int>(line);
                    out_location.range.end.character = static_cast<int>(col + identifier.size());
                    return true;
                }
            }
        }
        
        // Search event declarations
        for (size_t i = 0; i < doc->cached_ast->event_count; ++i) {
            const auto& event = doc->cached_ast->events[i];
            if (event.name == identifier) {
                size_t event_pos = find_in_source(source, identifier);
                if (event_pos != std::string_view::npos) {
                    auto [line, col] = offset_to_position(source, event_pos);
                    out_location.uri = uri;
                    out_location.range.start.line = static_cast<int>(line);
                    out_location.range.start.character = static_cast<int>(col);
                    out_location.range.end.line = static_cast<int>(line);
                    out_location.range.end.character = static_cast<int>(col + identifier.size());
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void analyze_document(Document& doc) {
        if (doc.text.empty()) {
            return; // No diagnostics for empty document
        }
        
        // Check if we need to parse (cache invalid or source changed)
        std::string_view current_source = doc.text;
        bool need_parse = !doc.cache_valid || doc.cached_source != current_source;
        
        if (need_parse) {
            // Parse and cache using forma parser
            std::cerr << "  Debug: Parsing document, source length=" << doc.cached_source.size() << std::endl;
            auto parsed = forma::parse_document(doc.cached_source);
            if (!doc.cached_ast) {
                doc.cached_ast = std::make_unique<forma::Document<32, 16, 16, 32, 64, 64>>();
            }
            *doc.cached_ast = parsed;
            doc.cache_valid = true;
            std::cerr << "  Debug: Parsed - types=" << doc.cached_ast->type_count 
                     << ", enums=" << doc.cached_ast->enum_count 
                     << ", events=" << doc.cached_ast->event_count 
                     << ", symbols=" << doc.cached_ast->symbols.count << std::endl;
        }
        
        // Run semantic analysis using forma::analyze_document
        // Note: LSP doesn't need import resolution or asset collection for basic diagnostics
        // Those would require file system access and slow down interactive editing
        if (doc.cached_ast) {
            auto sem_diagnostics = forma::analyze_document(*doc.cached_ast);
            convert_diagnostics(doc, sem_diagnostics);
        }
    }
    
private:
    // Find first occurrence of identifier in source (simple search)
    static size_t find_in_source(std::string_view source, std::string_view identifier) {
        size_t pos = 0;
        while (pos < source.size()) {
            size_t found = source.find(identifier, pos);
            if (found == std::string_view::npos) {
                return std::string_view::npos;
            }
            
            // Check if it's a standalone identifier (not part of another word)
            bool valid_start = (found == 0) || !is_identifier_char(source[found - 1]);
            bool valid_end = (found + identifier.size() >= source.size()) || 
                            !is_identifier_char(source[found + identifier.size()]);
            
            if (valid_start && valid_end) {
                return found;
            }
            
            pos = found + 1;
        }
        return std::string_view::npos;
    }
    
    // Convert offset to line/column position
    static std::pair<size_t, size_t> offset_to_position(std::string_view source, size_t offset) {
        size_t line = 0;
        size_t col = 0;
        
        for (size_t i = 0; i < offset && i < source.size(); ++i) {
            if (source[i] == '\n') {
                line++;
                col = 0;
            } else {
                col++;
            }
        }
        
        return {line, col};
    }
    
    // Extract identifier at a given position from source text
    static std::string_view extract_identifier_at_position(std::string_view source, Position pos) {
        // Convert line/character position to offset
        size_t offset = 0;
        int current_line = 0;
        int current_char = 0;
        
        for (size_t i = 0; i < source.size(); ++i) {
            if (current_line == pos.line && current_char == pos.character) {
                offset = i;
                break;
            }
            
            if (source[i] == '\n') {
                current_line++;
                current_char = 0;
            } else {
                current_char++;
            }
        }
        
        // If we didn't find the position, try end of source
        if (current_line < pos.line || (current_line == pos.line && current_char < pos.character)) {
            std::cerr << "  Debug: Position out of bounds (line=" << current_line << ", char=" << current_char << ")" << std::endl;
            return "";
        }
        
        std::cerr << "  Debug: Offset=" << offset << ", char at offset='" << (offset < source.size() ? source[offset] : '?') << "'" << std::endl;
        
        // Find the start of the identifier (move backward)
        size_t start = offset;
        while (start > 0 && is_identifier_char(source[start - 1])) {
            start--;
        }
        
        // Find the end of the identifier (move forward)
        size_t end = offset;
        while (end < source.size() && is_identifier_char(source[end])) {
            end++;
        }
        
        std::cerr << "  Debug: start=" << start << ", end=" << end << std::endl;
        
        if (start >= end) {
            std::cerr << "  Debug: start >= end, returning empty" << std::endl;
            return "";
        }
        
        return source.substr(start, end - start);
    }
    
    static bool is_identifier_char(char c) {
        return (c >= 'a' && c <= 'z') || 
               (c >= 'A' && c <= 'Z') || 
               (c >= '0' && c <= '9') || 
               c == '_';
    }
    
    template<size_t N>
    void convert_diagnostics(Document& doc, const forma::DiagnosticList<N>& sem_diagnostics) {
        doc.diagnostic_count = 0;
        
        // Convert semantic diagnostics to LSP diagnostics
        for (size_t i = 0; i < sem_diagnostics.count && doc.diagnostic_count < doc.diagnostics.size(); ++i) {
            const auto& sem_diag = sem_diagnostics.diagnostics[i];
            
            Diagnostic lsp_diag;
            lsp_diag.range.start.line = static_cast<int>(sem_diag.location.line);
            lsp_diag.range.start.character = static_cast<int>(sem_diag.location.column);
            lsp_diag.range.end.line = static_cast<int>(sem_diag.location.line);
            lsp_diag.range.end.character = static_cast<int>(sem_diag.location.column + sem_diag.location.length);
            
            // Map severity
            lsp_diag.severity = (sem_diag.severity == forma::DiagnosticSeverity::Error) 
                ? DiagnosticSeverity::Error 
                : DiagnosticSeverity::Warning;
            
            lsp_diag.code = sem_diag.code;
            lsp_diag.message = sem_diag.message;
            
            doc.diagnostics[doc.diagnostic_count++] = lsp_diag;
        }
    }
};

} // namespace forma::lsp
