#pragma once
#include <cstdint>

#ifndef FORMA_PLUGIN_API
#define FORMA_PLUGIN_API
#endif

// Forward declarations
struct RendererContext;
struct IRDocument;
struct IRUpdate;
struct IdeContext;
struct AudioContext;
struct AudioBuffer;
struct BuildContext;

enum class DiagnosticLevel {
    Info,
    Warning,
    Error
};

struct BuildDiagnostic {
    const char* message;
    DiagnosticLevel level;
    int line;
    int column;
    const char* file;
};

enum class PluginCapability {
    Renderer,
    Theme,
    ApiProfile,
    Build,
    Runtime,
    Tooling,
    IdeProtocol
};

struct RendererCapabilities {
  bool has_color;
  bool has_alpha;
  bool has_animation;
  bool has_pointer_input;
  bool has_font_metrics;
};
struct PluginCapabilities {
    bool supports_renderer;
    bool supports_theme;
    bool supports_audio;
    bool supports_build;
    
    // Renderer callback: render(doc, input_path, output_path) -> success
    bool (*render)(const void* doc, const char* input_path, const char* output_path);
    
    // Output file extension for renderer (e.g., ".c", ".cpp", ".js")
    const char* output_extension;
};


struct RendererVTable {
    void (*init)(RendererContext*);
    void (*load_document)(IRDocument*);
    void (*update)(IRUpdate*);
    void (*shutdown)();
};

struct IdeAdapterVTable {
    void (*initialize)(IdeContext*);
    void (*open_document)(const char* uri, const char* text);
    void (*change_document)(...);
    void (*request_completion)(...);
    void (*request_diagnostics)(...);
};
struct AudioVTable {
    void (*init)(AudioContext*);
    void (*process)(AudioBuffer*);
    void (*shutdown)();
};

struct BuildVTable {
    void (*init)(BuildContext*);
    void (*compile)(const char* source_file, const char* output_file);
    void (*link)(const char** object_files, int count, const char* output_binary);
    void (*clean)();
    void (*get_diagnostics)(BuildDiagnostic** diagnostics, int* count);
    void (*shutdown)();
};

struct FormaHost {
    void register_renderer(RendererVTable*);
    void register_audio(AudioVTable*);
    void register_build(BuildVTable*);
};


struct FormaPluginDescriptor {
    uint32_t api_version;
    const char* name;
    const char* version;
    PluginCapabilities capabilities;
    void (*register_plugin)(FormaHost*);
};

// Plugin initialization function
extern "C" FORMA_PLUGIN_API
const FormaPluginDescriptor* forma_plugin_init(uint32_t api_version);
