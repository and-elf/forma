#include "src/parser/ir.hpp"
#include "src/parser/semantic.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace forma;

std::string read_file(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

const char* severity_to_string(DiagnosticSeverity sev) {
    switch (sev) {
        case DiagnosticSeverity::Error: return "ERROR";
        case DiagnosticSeverity::Warning: return "WARNING";
        case DiagnosticSeverity::Info: return "INFO";
        default: return "UNKNOWN";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.forma>\n";
        return 1;
    }
    
    std::string source = read_file(argv[1]);
    std::cout << "Analyzing: " << argv[1] << "\n\n";
    
    // Parse the document
    auto doc = parse_document(source);
    
    std::cout << "Parsed:\n";
    std::cout << "  " << doc.type_count << " type declarations\n";
    std::cout << "  " << doc.instances.count << " instances\n\n";
    
    // Run semantic analysis
    auto diagnostics = analyze_document(doc);
    
    if (diagnostics.count == 0) {
        std::cout << "✓ No errors found - all property types are correct!\n";
        return 0;
    }
    
    std::cout << "Found " << diagnostics.count << " diagnostic(s):\n\n";
    
    for (size_t i = 0; i < diagnostics.count; ++i) {
        const auto& diag = diagnostics.diagnostics[i];
        std::cout << severity_to_string(diag.severity) << ": "
                  << std::string_view(diag.message.data(), diag.message.size())
                  << " at offset " << diag.location.offset
                  << " (code: " << std::string_view(diag.code.data(), diag.code.size()) << ")\n";
    }
    
    return diagnostics.count > 0 ? 1 : 0;
}
