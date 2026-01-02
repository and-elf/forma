// Example Plugin: Hello World
// Demonstrates dynamic plugin loading

#include "../../src/plugin_hash.hpp"
#include <cstdint>
#include <iostream>

// Plugin metadata hash - computed from plugin.toml at compile time
constexpr std::string_view PLUGIN_TOML_CONTENT = R"(# Hello World Example Plugin

[plugin]
name = "hello-world"
kind = "example"
api_version = "1.0.0"
runtime = "native"

[capabilities]
provides = [
    "example",
    "demo"
]

dependencies = []
)";

constexpr uint64_t METADATA_HASH = forma::fnv1a_hash(PLUGIN_TOML_CONTENT);

extern "C" {

// Plugin metadata hash (required)
uint64_t forma_plugin_metadata_hash() {
    return METADATA_HASH;
}

// Render function (required but does nothing for example plugin)
bool forma_render(const void* doc_ptr, const char* input_path, const char* output_path) {
    (void)doc_ptr;
    (void)input_path;
    (void)output_path;
    
    std::cout << "[Hello Plugin] Render called (but this is just an example plugin)\n";
    return true;
}

// Optional registration
void forma_register(void* host) {
    (void)host;
    std::cout << "[Hello Plugin] Plugin registered!\n";
}

// Host-aware registration example that can use HostContext
void forma_register_host(void* host_ptr) {
    auto* host = static_cast<forma::HostContext*>(host_ptr);
    (void)host;
    std::cout << "[Hello Plugin] Plugin registered with host!\n";
}

} // extern "C"
