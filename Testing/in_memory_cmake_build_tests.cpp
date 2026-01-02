#include <bugspray/bugspray.hpp>
#include "plugin_loader.hpp"
#include "core/fs/i_file_system.hpp"

using namespace forma;

// Mock build function that simulates cmake build: writes build/app file and CMakeLists.txt
extern "C" int mock_cmake_build(const char* project_dir, const char* /*config_path*/, bool /*verbose*/, bool /*flash*/, bool /*monitor*/) {
    if (!project_dir) return 1;
    std::filesystem::path proj(project_dir);
    try {
        std::filesystem::create_directories(proj / "build");
        std::ofstream of((proj / "build" / "app").string(), std::ios::binary);
        of << "BINARY";
        of.close();
        std::ofstream cm((proj / "CMakeLists.txt").string());
        cm << "# Generated CMakeLists";
        cm.close();
        return 0;
    } catch (...) {
        return 1;
    }
}

static std::unique_ptr<PluginMetadata> make_cmake_build_metadata() {
    auto m = std::make_unique<PluginMetadata>();
    m->name = "mock-cmake-generator";
    m->kind = "build";
    m->api_version = "1.0.0";
    m->runtime = "native";
    m->provides = {"build:cmake"};
    return m;
}

TEST_CASE("InMemory - CMake build flow using MemoryFileSystem") {
    PluginLoader loader;

    // Register builtin build plugin
    loader.register_builtin_plugin(nullptr, mock_cmake_build, nullptr, make_cmake_build_metadata());

    auto& plugins = const_cast<std::vector<std::unique_ptr<LoadedPlugin>>&>(loader.get_loaded_plugins());
    REQUIRE(!plugins.empty());
    auto& loaded = plugins.back();

    // Provide a build_adapter that mirrors behavior of dynamic plugin adapter
    loaded->functions.build = mock_cmake_build;
    loaded->build_adapter = [bf = loaded->functions.build, p = loaded.get()](const std::string& project_dir, const std::string& config_path, forma::fs::IFileSystem& fs, bool verbose, bool flash, bool monitor) -> int {
        // Create temp on-disk project, call build, then copy outputs back into fs
        auto tmp_proj = std::filesystem::temp_directory_path() / ("test_cmake_proj_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
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

        // Copy disk outputs back into fs under project_dir
        forma::fs::copy_disk_to_fs(tmp_proj.string(), fs, project_dir);

        std::error_code ec; std::filesystem::remove_all(tmp_proj, ec);
        return rc;
    };

    // Attach MemoryFileSystem as HostContext
    auto memfs = std::make_unique<fs::MemoryFileSystem>();
    auto ctx = std::make_unique<HostContext>(std::move(memfs), nullptr);
    ctx->initialize_stream_io();
    loader.set_host_context(std::move(ctx));

    HostContext* hc = loader.get_host_context();
    REQUIRE(hc != nullptr);

    // Prepare a simple virtual project in memory: src/main.c and CMakeLists
    std::string project_root = "memproj";
    hc->filesystem->create_dirs(project_root + "/src");
    hc->filesystem->write_file(project_root + "/src/main.c", "int main(){return 0;}\n");

    // Call build adapter
    int rc = loaded->build_adapter(project_root, "", *hc->filesystem, false, false, false);
    CHECK(rc == 0);

    // Verify outputs in memory filesystem
    CHECK(hc->filesystem->exists(project_root + "/build/app"));
    auto content = hc->filesystem->read_file(project_root + "/build/app");
    CHECK(content == "BINARY");
    CHECK(hc->filesystem->exists(project_root + "/CMakeLists.txt"));
}
