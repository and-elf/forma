#include <bugspray/bugspray.hpp>
#include "core/fs/i_file_system.hpp"
#include "commands/init.hpp"
#include "plugin_loader.hpp"

using namespace forma;

// Mock renderer: writes generated C source to output_path
extern "C" bool mock_renderer(const void* /*doc*/, const char* input_path, const char* output_path) {
    if (!input_path || !output_path) return false;
    try {
        std::ifstream in(input_path, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), {});
        in.close();
        std::string out = "// Generated C source\n" + content;
        std::ofstream of(output_path, std::ios::binary);
        of << out;
        of.close();
        return true;
    } catch (...) { return false; }
}

// Mock build: reads project_dir/src/main.c and writes build/app
extern "C" int mock_build(const char* project_dir, const char* /*config_path*/, bool /*verbose*/, bool /*flash*/, bool /*monitor*/) {
    if (!project_dir) return 1;
    try {
        std::filesystem::path proj(project_dir);
        std::filesystem::create_directories(proj / "build");
        std::ofstream of((proj / "build" / "app").string(), std::ios::binary);
        of << "BINARY";
        of.close();
        return 0;
    } catch (...) { return 1; }
}

static std::unique_ptr<PluginMetadata> make_mock_renderer_metadata() {
    auto m = std::make_unique<PluginMetadata>();
    m->name = "mock-renderer";
    m->kind = "renderer";
    m->api_version = "1.0.0";
    m->runtime = "native";
    m->provides = {"renderer:mock"};
    m->output_extension = ".c";
    m->output_language = "c";
    return m;
}

static std::unique_ptr<PluginMetadata> make_mock_build_metadata() {
    auto m = std::make_unique<PluginMetadata>();
    m->name = "mock-builder";
    m->kind = "build";
    m->api_version = "1.0.0";
    m->runtime = "native";
    m->provides = {"build:cmake"};
    return m;
}

TEST_CASE("Full in-memory integration: init -> render -> build") {
    PluginLoader loader;

    // Register builtin mock plugins
    loader.register_builtin_plugin(mock_renderer, nullptr, nullptr, make_mock_renderer_metadata());
    loader.register_builtin_plugin(nullptr, mock_build, nullptr, make_mock_build_metadata());

    // Replace builtin builder's adapter with an in-memory aware adapter that mirrors fs <-> disk
    {
        auto* p = loader.find_plugin("mock-builder");
        REQUIRE(p != nullptr);
        p->functions.build = mock_build; // ensure pointer is set
        // Create an adapter similar to PluginLoader's dynamic plugin adapter
        HostContext* hc = loader.get_host_context();
        p->build_adapter = [bf = p->functions.build, hc](const std::string& project_dir, const std::string& config_path, forma::fs::IFileSystem& fs, bool verbose, bool flash, bool monitor) -> int {
            try {
                // Create a temp project directory on disk
                std::filesystem::path tmp_proj = std::filesystem::temp_directory_path() / ("test_cmake_proj_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
                std::filesystem::create_directories(tmp_proj);

                // If config exists in fs, write it out
                try {
                    if (!config_path.empty() && fs.exists(config_path)) {
                        auto cfg = fs.read_file(config_path);
                        std::ofstream cof((tmp_proj / "forma_plugin_config.toml").string(), std::ios::binary);
                        cof << cfg;
                        cof.close();
                    }
                } catch(...) {}

                // Copy fs -> disk under project_dir path
                forma::fs::copy_fs_to_disk(fs, project_dir, tmp_proj.string());

                int rc = bf(tmp_proj.string().c_str(), nullptr, verbose, flash, monitor);

                // Copy disk outputs back into HostContext filesystem
                if (hc && hc->filesystem) {
                    forma::fs::copy_disk_to_fs(tmp_proj.string(), *hc->filesystem, project_dir);
                }

                // Copy disk outputs back into caller fs
                try {
                    forma::fs::copy_disk_to_fs(tmp_proj.string(), fs, project_dir);
                } catch(...) {}

                std::error_code ec; std::filesystem::remove_all(tmp_proj, ec);
                return rc;
            } catch (...) {
                return -1;
            }
        };
    }

    // Prepare an in-memory HostContext
    auto memfs = std::make_unique<fs::MemoryFileSystem>();
    auto ctx = std::make_unique<HostContext>(std::move(memfs), nullptr);
    ctx->initialize_stream_io();
    loader.set_host_context(std::move(ctx));

    HostContext* hc = loader.get_host_context();
    REQUIRE(hc != nullptr);

    // Run init into memory
    commands::InitOptions opts;
    opts.project_name = "testapp";
    opts.project_dir = "memproj";
    opts.renderer = "mock";
    int rc = commands::run_init_command(opts, *hc->filesystem);
    CHECK(rc == 0);

    // Ensure main.forma exists in memory
    CHECK(hc->filesystem->exists("memproj/src/main.forma"));

    // Simulate rendering step: call renderer adapter to generate C source at src/main.c
    auto renderer = loader.get_renderer_adapter("mock-renderer");
    REQUIRE(static_cast<bool>(renderer));

    // We'll render into memproj/src/main.c
    bool ok = renderer(nullptr, "memproj/src/main.forma", "memproj/src/main.c", *hc->filesystem);
    CHECK(ok);
    CHECK(hc->filesystem->exists("memproj/src/main.c"));

    // Now call build adapter for mock-builder
    auto builder = loader.get_build_adapter("mock-builder");
    REQUIRE(static_cast<bool>(builder));

    int build_rc = builder("memproj", "", *hc->filesystem, false, false, false);
    CHECK(build_rc == 0);

    // Check produced binary in memory
    CHECK(hc->filesystem->exists("memproj/build/app"));
    auto bin = hc->filesystem->read_file("memproj/build/app");
    CHECK(bin == "BINARY");
}
