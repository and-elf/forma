#pragma once
#include <array>
#include <string_view>

namespace forma {

// ============================================================================
// Diagnostics for LSP and Semantic Analysis
// ============================================================================

enum class DiagnosticSeverity {
    Error,
    Warning,
    Info,
    Hint
};

struct SourceLocation {
    size_t line = 0;
    size_t column = 0;
    size_t offset = 0;
    size_t length = 0;
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string_view message;
    SourceLocation location;
    std::string_view code;  // e.g., "unknown-type", "duplicate-declaration"
};

template <size_t N>
struct DiagnosticList {
    std::array<Diagnostic, N> diagnostics{};
    size_t count = 0;
    
    constexpr void add(DiagnosticSeverity severity, std::string_view message, 
                       SourceLocation loc, std::string_view code = "") {
        if (count < N) {
            diagnostics[count++] = Diagnostic{severity, message, loc, code};
        }
    }
    
    constexpr bool has_errors() const {
        for (size_t i = 0; i < count; ++i) {
            if (diagnostics[i].severity == DiagnosticSeverity::Error) {
                return true;
            }
        }
        return false;
    }
};

} // namespace forma
