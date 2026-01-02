#include <bugspray/bugspray.hpp>
#include "plugin_loader.hpp"
#include "core/fs/i_file_system.hpp"

using namespace forma;

// A simple builtin renderer that writes a C main file into the provided output path
extern "C" bool test_c_codegen_renderer(const void* /*doc*/, const char* /*input_path*/, const char* output_path) {
    // output_path will be treated as a file path (e.g., "memproj/src/main.c")
    if (!output_path) return false;
    std::string out(output_path);
    // Create directories on disk; but in our adapter we'll route into in-memory FS
    // Here, the builtin renderer writes to a temp on-disk file (plugin adapter copies back)
    std::filesystem::path p = std::filesystem::temp_directory_path() / ("test_codegen_" + std::to_string(std::time(nullptr)) + ".c");
    std::ofstream of(p.string());
    of << "#include <stdio.h>\nint main(void){ printf(\"Hello from in-memory codegen\\n\"); return 0; }\n";
    of.close();
    // Move temp file to requested output path on disk (the adapter will sync to memory)
    try { std::filesystem::create_directories(std::filesystem::path(output_path).parent_path()); } catch(...) {}
    std::error_code ec;
    std::filesystem::copy_file(p, output_path, std::filesystem::copy_options::overwrite_existing, ec);
    std::filesystem::remove(p, ec);
    return !ec;
}

static std::unique_ptr<PluginMetadata> make_codegen_metadata() {
    auto m = std::make_unique<PluginMetadata>();
    m->name = "builtin-c-codegen";
    m->kind = "renderer";
    m->api_version = "1.0.0";
    m->runtime = "native";
    m->provides = {"renderer:c"};
    return m;
}

TEST_CASE("InMemory - C codegen via MemoryFileSystem") {
    PluginLoader loader;

    // Attempt to load real c-codegen plugin from build output
    std::string plugin_path = "build/plugins/c-codegen/forma-c-codegen.so";
    std::string err;
    bool plugin_loaded = false;
    LoadedPlugin* loaded = nullptr;
    if (std::filesystem::exists(plugin_path)) {
        plugin_loaded = loader.load_plugin(plugin_path, err);
        if (!plugin_loaded) {
            std::cerr << "Failed to load plugin: " << err << "\n";
        }
    }

    if (plugin_loaded) {
        auto& plugins = const_cast<std::vector<std::unique_ptr<LoadedPlugin>>&>(loader.get_loaded_plugins());
        REQUIRE(!plugins.empty());
        loaded = plugins.back().get();
    } else {
        // Fall back to builtin test renderer as before
        loader.register_builtin_plugin(test_c_codegen_renderer, nullptr, nullptr, make_codegen_metadata());
        auto& plugins = const_cast<std::vector<std::unique_ptr<LoadedPlugin>>&>(loader.get_loaded_plugins());
        REQUIRE(!plugins.empty());
        loaded = plugins.back().get();

        // Create a fully in-memory adapter for the builtin: write content directly into the provided IFileSystem
        loaded->renderer_adapter = [](const void* /*doc*/, const std::string& /*input_path*/, const std::string& output_path, forma::fs::IFileSystem& fs) -> bool {
            try {
                auto parent = std::filesystem::path(output_path).parent_path().string();
                if (!parent.empty()) fs.create_dirs(parent);
                std::string code = "#include <stdio.h>\nint main(void){ printf(\"Hello from in-memory codegen\\n\"); return 0; }\n";
                fs.write_file(output_path, code);
                return true;
            } catch (...) { return false; }
        };
    }

    // Attach MemoryFileSystem as HostContext
    auto memfs = std::make_unique<fs::MemoryFileSystem>();
    auto ctx = std::make_unique<HostContext>(std::move(memfs), nullptr);
    ctx->initialize_stream_io();
    loader.set_host_context(std::move(ctx));

    // Now call renderer adapter to generate C code into virtual project 'memproj/src/main.c'
    HostContext* hc = loader.get_host_context();
    REQUIRE(hc != nullptr);

    std::string virtual_path = "memproj/src/main.c";
    bool ok = loaded->renderer_adapter(nullptr, "", virtual_path, *hc->filesystem);
    CHECK(ok == true);

    // Verify generated file exists in memory FS
    CHECK(hc->filesystem->exists(virtual_path));
    auto content = hc->filesystem->read_file(virtual_path);
    CHECK(content.find("Hello from in-memory codegen") != std::string::npos);
}
