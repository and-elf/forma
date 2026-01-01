#include <bugspray/bugspray.hpp>
#include "plugin_loader.hpp"
#include "core/fs/i_file_system.hpp"

using namespace forma;

// A simple builtin build function that writes a file into the provided project dir
extern "C" int test_builtin_build(const char* project_dir, const char* /*config_path*/, bool /*verbose*/, bool /*flash*/, bool /*monitor*/) {
    // Create a file under project_dir/output.bin
    std::filesystem::path p(project_dir);
    std::filesystem::create_directories(p);
    std::ofstream of(p / "output.bin", std::ios::binary);
    of << "BINARYDATA";
    of.close();
    return 0;
}

// A fake metadata loader to produce PluginMetadata for the builtin
static std::unique_ptr<PluginMetadata> make_test_metadata() {
    auto m = std::make_unique<PluginMetadata>();
    m->name = "test-build";
    m->kind = "build";
    m->api_version = "1.0.0";
    m->runtime = "native";
    m->provides = {"build"};
    return m;
}

TEST_CASE("PluginLoader - BuiltinBuildSyncsToMemoryFS")
{
    PluginLoader loader;

    // Register builtin plugin (provide no render_fn, provide build_fn)
    loader.register_builtin_plugin(nullptr, test_builtin_build, nullptr, make_test_metadata());

    auto& plugins = const_cast<std::vector<std::unique_ptr<LoadedPlugin>>&>(loader.get_loaded_plugins());
    REQUIRE(!plugins.empty());
    auto& loaded = plugins.back();

    // Overwrite functions.build and build_adapter
    loaded->functions.build = test_builtin_build;
    loaded->build_adapter = [ptr = loaded.get()](const std::string& project_dir, const std::string& /*config_path*/, forma::fs::IFileSystem& /*fs*/, bool /*verbose*/, bool /*flash*/, bool /*monitor*/) -> int {
        // Call the C function with a temp on-disk project dir
        std::filesystem::path tmp = std::filesystem::temp_directory_path() / ("test_plugin_proj_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tmp);
        int rc = ptr->functions.build(tmp.string().c_str(), nullptr, false, false, false);

        // Sync files back to host_context filesystem if present
        if (ptr->host_context && ptr->host_context->filesystem) {
            auto& tfs = *ptr->host_context->filesystem;
            for (auto& p : std::filesystem::recursive_directory_iterator(tmp)) {
                if (p.is_regular_file()) {
                    auto rel = std::filesystem::relative(p.path(), tmp).string();
                    std::string dest = project_dir + "/" + rel;
                    std::ifstream inf(p.path(), std::ios::binary);
                    std::string content((std::istreambuf_iterator<char>(inf)), {});
                    inf.close();
                    tfs.create_dirs(std::filesystem::path(dest).parent_path().string());
                    tfs.write_file(dest, content);
                }
            }
        }

        std::error_code ec;
        std::filesystem::remove_all(tmp, ec);
        return rc;
    };

    // Attach a MemoryFileSystem as the plugin's HostContext filesystem
    auto memfs = std::make_unique<fs::MemoryFileSystem>();
    auto ctx = std::make_unique<HostContext>(std::move(memfs), nullptr);
    loaded->host_context = std::move(ctx);

    // Now call build adapter to build project 'memproj'
    int rc = loaded->build_adapter("memproj", "", *loaded->host_context->filesystem, false, false, false);
    CHECK(rc == 0);

    // Verify MemoryFileSystem has the synced file
    auto& fs = *loaded->host_context->filesystem;
    CHECK(fs.exists("memproj/output.bin"));
    auto content = fs.read_file("memproj/output.bin");
    CHECK(content == "BINARYDATA");
}
