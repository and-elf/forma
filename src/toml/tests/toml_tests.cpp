#include <bugspray/bugspray.hpp>
#include "toml.hpp"

using namespace forma::toml;

TEST_CASE("TOML - Basic Key-Value Parsing")
{
    constexpr auto doc = parse(R"(
name = "Forma"
version = "0.1.0"
year = 2025
active = true
)");
    
    SECTION("String values")
    {
        auto name = doc.root.get_string("name");
        REQUIRE(name.has_value());
        CHECK(name.value() == "Forma");
        
        auto version = doc.root.get_string("version");
        REQUIRE(version.has_value());
        CHECK(version.value() == "0.1.0");
    }
    
    SECTION("Integer values")
    {
        auto year = doc.root.get_int("year");
        REQUIRE(year.has_value());
        CHECK(year.value() == 2025);
    }
    
    SECTION("Boolean values")
    {
        auto active = doc.root.get_bool("active");
        REQUIRE(active.has_value());
        CHECK(active.value() == true);
    }
}

TEST_CASE("TOML - Table Sections")
{
    constexpr auto doc = parse(R"(
[package]
name = "forma-lsp"
version = "0.1.0"

[dependencies]
cpp-standard = "20"
)");
    
    SECTION("Package table")
    {
        auto package = doc.get_table("package");
        REQUIRE(package != nullptr);
        
        auto name = package->get_string("name");
        REQUIRE(name.has_value());
        CHECK(name.value() == "forma-lsp");
        
        auto version = package->get_string("version");
        REQUIRE(version.has_value());
        CHECK(version.value() == "0.1.0");
    }
    
    SECTION("Dependencies table")
    {
        auto deps = doc.get_table("dependencies");
        REQUIRE(deps != nullptr);
        
        auto cpp = deps->get_string("cpp-standard");
        REQUIRE(cpp.has_value());
        CHECK(cpp.value() == "20");
    }
}

TEST_CASE("TOML - Comments")
{
    constexpr auto doc = parse(R"(
# This is a comment
name = "test"  # inline comment
# Another comment
version = "1.0"
)");
    
    auto name = doc.root.get_string("name");
    REQUIRE(name.has_value());
    CHECK(name.value() == "test");
    
    auto version = doc.root.get_string("version");
    REQUIRE(version.has_value());
    CHECK(version.value() == "1.0");
}

TEST_CASE("TOML - Integer Types")
{
    constexpr auto doc = parse(R"(
positive = 42
negative = -17
zero = 0
)");
    
    SECTION("Positive integers")
    {
        auto pos = doc.root.get_int("positive");
        REQUIRE(pos.has_value());
        CHECK(pos.value() == 42);
    }
    
    SECTION("Negative integers")
    {
        auto neg = doc.root.get_int("negative");
        REQUIRE(neg.has_value());
        CHECK(neg.value() == -17);
    }
    
    SECTION("Zero")
    {
        auto z = doc.root.get_int("zero");
        REQUIRE(z.has_value());
        CHECK(z.value() == 0);
    }
}

TEST_CASE("TOML - Boolean Types")
{
    constexpr auto doc = parse(R"(
enabled = true
disabled = false
)");
    
    auto enabled = doc.root.get_bool("enabled");
    REQUIRE(enabled.has_value());
    CHECK(enabled.value() == true);
    
    auto disabled = doc.root.get_bool("disabled");
    REQUIRE(disabled.has_value());
    CHECK(disabled.value() == false);
}

TEST_CASE("TOML - Nested Tables")
{
    constexpr auto doc = parse(R"(
[server]
host = "localhost"
port = 8080

[server.logging]
level = "debug"
enabled = true
)");
    
    auto server = doc.get_table("server");
    REQUIRE(server != nullptr);
    
    auto host = server->get_string("host");
    REQUIRE(host.has_value());
    CHECK(host.value() == "localhost");
    
    auto logging = doc.get_table("server.logging");
    REQUIRE(logging != nullptr);
    
    auto level = logging->get_string("level");
    REQUIRE(level.has_value());
    CHECK(level.value() == "debug");
}

TEST_CASE("TOML - Empty Values")
{
    constexpr auto doc = parse(R"(
empty = ""
)");
    
    auto empty = doc.root.get_string("empty");
    REQUIRE(empty.has_value());
    CHECK(empty.value() == "");
}

TEST_CASE("TOML - Missing Keys")
{
    constexpr auto doc = parse(R"(
name = "test"
)");
    
    auto missing = doc.root.get_string("nonexistent");
    CHECK(!missing.has_value());
    
    auto missing_int = doc.root.get_int("nonexistent");
    CHECK(!missing_int.has_value());
}

TEST_CASE("TOML - Array Values")
{
    constexpr auto doc = parse(R"(
[imports]
paths = ["./lib", "./lib/forma", "/usr/lib/forma"]

[plugins]
enabled_list = ["cmake", "vscode", "lsp"]
)");
    
    SECTION("Import paths array")
    {
        auto imports = doc.get_table("imports");
        REQUIRE(imports != nullptr);
        
        auto idx = imports->get_array_index("paths");
        REQUIRE(idx != static_cast<size_t>(-1));
        
        const auto& paths = doc.arrays[idx];
        CHECK(paths.count == 3);
        CHECK(paths.elements[0] == "./lib");
        CHECK(paths.elements[1] == "./lib/forma");
        CHECK(paths.elements[2] == "/usr/lib/forma");
    }
    
    SECTION("Plugins array")
    {
        auto plugins = doc.get_table("plugins");
        REQUIRE(plugins != nullptr);
        
        auto idx = plugins->get_array_index("enabled_list");
        REQUIRE(idx != static_cast<size_t>(-1));
        
        const auto& enabled = doc.arrays[idx];
        CHECK(enabled.count == 3);
        CHECK(enabled.elements[0] == "cmake");
        CHECK(enabled.elements[1] == "vscode");
        CHECK(enabled.elements[2] == "lsp");
    }
}
