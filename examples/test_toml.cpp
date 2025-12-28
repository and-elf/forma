#include "src/toml.hpp"
#include <iostream>
#include <cassert>

using namespace forma::toml;

void test_basic_parsing() {
    std::cout << "Test: Basic TOML Parsing\n";
    std::cout << "=========================\n";
    
    const char* toml = R"(
name = "Forma"
version = "0.1.0"
year = 2025
active = true
)";
    
    auto doc = parse(toml);
    
    auto name = doc.root.get_string("name");
    assert(name.has_value());
    assert(name.value() == "Forma");
    std::cout << "✓ name = \"" << name.value() << "\"\n";
    
    auto version = doc.root.get_string("version");
    assert(version.has_value());
    assert(version.value() == "0.1.0");
    std::cout << "✓ version = \"" << version.value() << "\"\n";
    
    auto year = doc.root.get_int("year");
    assert(year.has_value());
    assert(year.value() == 2025);
    std::cout << "✓ year = " << year.value() << "\n";
    
    auto active = doc.root.get_bool("active");
    assert(active.has_value());
    assert(active.value() == true);
    std::cout << "✓ active = " << (active.value() ? "true" : "false") << "\n\n";
}

void test_tables() {
    std::cout << "Test: TOML Tables\n";
    std::cout << "=================\n";
    
    const char* toml = R"(
[package]
name = "forma-lsp"
version = "0.1.0"

[dependencies]
cpp-standard = "20"
)";
    
    auto doc = parse(toml);
    
    auto package = doc.get_table("package");
    assert(package != nullptr);
    std::cout << "✓ Found [package] table\n";
    
    auto name = package->get_string("name");
    assert(name.has_value());
    assert(name.value() == "forma-lsp");
    std::cout << "  name = \"" << name.value() << "\"\n";
    
    auto deps = doc.get_table("dependencies");
    assert(deps != nullptr);
    std::cout << "✓ Found [dependencies] table\n";
    
    auto cpp = deps->get_string("cpp-standard");
    assert(cpp.has_value());
    std::cout << "  cpp-standard = \"" << cpp.value() << "\"\n\n";
}

void test_project_config() {
    std::cout << "Test: Project Configuration\n";
    std::cout << "============================\n";
    
    const char* toml = R"(
[project]
name = "forma"
version = "0.1.0"
description = "A QML-inspired programming language"
authors = "Andreas"

[build]
standard = "c++20"
warnings = true
optimize = true

[lsp]
port = 8080
diagnostics = true
max-documents = 16

[plugins]
enabled = true
directory = "./plugins"
)";
    
    auto doc = parse(toml);
    
    // Project section
    auto project = doc.get_table("project");
    assert(project != nullptr);
    std::cout << "✓ [project]\n";
    std::cout << "  name: " << project->get_string("name").value() << "\n";
    std::cout << "  version: " << project->get_string("version").value() << "\n";
    std::cout << "  description: " << project->get_string("description").value() << "\n";
    
    // Build section
    auto build = doc.get_table("build");
    assert(build != nullptr);
    std::cout << "\n✓ [build]\n";
    std::cout << "  standard: " << build->get_string("standard").value() << "\n";
    std::cout << "  warnings: " << (build->get_bool("warnings").value() ? "true" : "false") << "\n";
    std::cout << "  optimize: " << (build->get_bool("optimize").value() ? "true" : "false") << "\n";
    
    // LSP section
    auto lsp = doc.get_table("lsp");
    assert(lsp != nullptr);
    std::cout << "\n✓ [lsp]\n";
    std::cout << "  port: " << lsp->get_int("port").value() << "\n";
    std::cout << "  diagnostics: " << (lsp->get_bool("diagnostics").value() ? "true" : "false") << "\n";
    std::cout << "  max-documents: " << lsp->get_int("max-documents").value() << "\n";
    
    // Plugins section
    auto plugins = doc.get_table("plugins");
    assert(plugins != nullptr);
    std::cout << "\n✓ [plugins]\n";
    std::cout << "  enabled: " << (plugins->get_bool("enabled").value() ? "true" : "false") << "\n";
    std::cout << "  directory: " << plugins->get_string("directory").value() << "\n\n";
}

void test_forma_toml() {
    std::cout << "Test: Forma.toml Example\n";
    std::cout << "========================\n";
    
    const char* toml = R"(
[package]
name = "my-forma-app"
version = "1.0.0"
entry = "main.fml"

[dependencies]
forma-std = "0.1"
forma-ui = "0.2"

[build]
target = "native"
optimize = true

[dev-dependencies]
forma-test = "0.1"
)";
    
    auto doc = parse(toml);
    
    auto package = doc.get_table("package");
    assert(package != nullptr);
    std::cout << "Package: " << package->get_string("name").value() 
              << " v" << package->get_string("version").value() << "\n";
    std::cout << "Entry: " << package->get_string("entry").value() << "\n";
    
    auto deps = doc.get_table("dependencies");
    if (deps) {
        std::cout << "\nDependencies:\n";
        for (size_t i = 0; i < deps->entry_count; ++i) {
            const auto& entry = deps->entries[i];
            std::cout << "  " << entry.key << " = \"" << entry.value.string_value << "\"\n";
        }
    }
    
    auto build = doc.get_table("build");
    if (build) {
        std::cout << "\nBuild:\n";
        std::cout << "  target: " << build->get_string("target").value() << "\n";
        std::cout << "  optimize: " << (build->get_bool("optimize").value() ? "yes" : "no") << "\n";
    }
    
    std::cout << "\n";
}

void test_comments_and_whitespace() {
    std::cout << "Test: Comments and Whitespace\n";
    std::cout << "==============================\n";
    
    const char* toml = R"(
# Project configuration
name = "test"     # The project name

    # Build settings
    optimize = true
version = "1.0"
)";
    
    auto doc = parse(toml);
    
    assert(doc.root.get_string("name").value() == "test");
    assert(doc.root.get_bool("optimize").value() == true);
    assert(doc.root.get_string("version").value() == "1.0");
    
    std::cout << "✓ Comments handled correctly\n";
    std::cout << "✓ Whitespace handled correctly\n\n";
}

int main() {
    std::cout << "Forma TOML Parser Tests\n";
    std::cout << "=======================\n\n";
    
    test_basic_parsing();
    test_tables();
    test_project_config();
    test_forma_toml();
    test_comments_and_whitespace();
    
    std::cout << "====================================\n";
    std::cout << "✓ All TOML tests passed!\n";
    std::cout << "====================================\n";
    
    return 0;
}
