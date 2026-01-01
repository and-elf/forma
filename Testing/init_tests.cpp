#include <bugspray/bugspray.hpp>
#include "commands/init.hpp"
#include "core/fs/i_file_system.hpp"

using namespace forma::commands;

TEST_CASE("InitCommand - CreatesProjectFilesInMemoryFS")
{
    InitOptions opts;
    opts.project_name = "memproj";
    opts.project_dir = "memproj";
    opts.verbose = false;

    forma::fs::MemoryFileSystem mem;

    int rc = run_init_command(opts, mem);
    CHECK(rc == 0);

    CHECK(mem.exists("memproj/forma.toml"));
    CHECK(mem.exists("memproj/src/main.forma"));

    auto toml = mem.read_file("memproj/forma.toml");
    CHECK(toml.find("[package]") != std::string::npos);
}
