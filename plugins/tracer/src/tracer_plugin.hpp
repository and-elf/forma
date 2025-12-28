#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace forma::tracer {

// ============================================================================
// Trace Levels
// ============================================================================

enum class TraceLevel {
    Silent,   // No output except errors
    Normal,   // Stage markers only
    Verbose,  // Detailed statistics
    Debug     // Internal operations
};

// ============================================================================
// Tracer Plugin
// ============================================================================

class TracerPlugin {
private:
    TraceLevel level = TraceLevel::Normal;
    int indent = 0;
    bool in_stage = false;
    std::string current_stage;
    
    void print_indent() const {
        for (int i = 0; i < indent; ++i) {
            std::cout << "  ";
        }
    }

public:
    // Configuration
    void set_level(TraceLevel lvl) {
        level = lvl;
    }

    TraceLevel get_level() const {
        return level;
    }

    // Stage tracking
    void begin_stage(std::string_view stage_name) {
        if (level == TraceLevel::Silent) return;
        
        current_stage = stage_name;
        in_stage = true;
        
        print_indent();
        std::cout << "▶ " << stage_name << "\n";
        indent++;
    }

    void end_stage() {
        if (level == TraceLevel::Silent) return;
        if (!in_stage) return;
        
        indent--;
        print_indent();
        std::cout << "✓ " << current_stage << " complete\n";
        
        in_stage = false;
        current_stage.clear();
    }

    // Output methods
    void info(std::string_view message) {
        if (level == TraceLevel::Silent) return;
        
        print_indent();
        std::cout << message << "\n";
    }

    void verbose(std::string_view message) {
        if (level < TraceLevel::Verbose) return;
        
        print_indent();
        std::cout << message << "\n";
    }

    void debug(std::string_view message) {
        if (level < TraceLevel::Debug) return;
        
        print_indent();
        std::cout << "  [DEBUG] " << message << "\n";
    }

    void error(std::string_view message) {
        // Always show errors
        print_indent();
        std::cout << "  ✗ ERROR: " << message << "\n";
    }

    void warning(std::string_view message) {
        if (level == TraceLevel::Silent) return;
        
        print_indent();
        std::cout << "  ⚠ WARNING: " << message << "\n";
    }

    void stat(std::string_view key, std::string_view value) {
        if (level < TraceLevel::Verbose) return;
        
        print_indent();
        std::cout << "  " << key << ": " << value << "\n";
    }

    void stat(std::string_view key, int value) {
        if (level < TraceLevel::Verbose) return;
        
        print_indent();
        std::cout << "  " << key << ": " << value << "\n";
    }

    void success(std::string_view message) {
        if (level == TraceLevel::Silent) return;
        
        std::cout << "\n✓ " << message << "\n";
    }

    void failure(std::string_view message) {
        // Always show failures
        std::cout << "\n✗ " << message << "\n";
    }
};

// ============================================================================
// Plugin Registration
// ============================================================================

// Global tracer instance (singleton pattern)
inline TracerPlugin* g_tracer_instance = nullptr;

inline TracerPlugin& get_tracer() {
    if (!g_tracer_instance) {
        static TracerPlugin default_tracer;
        g_tracer_instance = &default_tracer;
    }
    return *g_tracer_instance;
}

inline void set_tracer(TracerPlugin* tracer) {
    g_tracer_instance = tracer;
}

// Plugin metadata
inline const char* get_plugin_name() {
    return "tracer";
}

inline const char* get_plugin_version() {
    return "0.1.0";
}

} // namespace forma::tracer
