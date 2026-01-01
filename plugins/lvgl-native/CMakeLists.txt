cmake_minimum_required(VERSION 3.20)
project(forma_lvgl_native)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Plugin sources
set(PLUGIN_SOURCES
    src/lvgl_native_renderer.cpp
    src/sdl3_downloader.cpp
    src/lvgl_downloader.cpp
)

# Create shared library plugin
add_library(forma_lvgl_native SHARED ${PLUGIN_SOURCES})

# Include directories
target_include_directories(forma_lvgl_native PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/plugins/lvgl-renderer/src
)

# Link against archive-utils and http-client
target_link_libraries(forma_lvgl_native PRIVATE
    forma_archive_utils
    forma_http_client
)

# Set output name
set_target_properties(forma_lvgl_native PROPERTIES
    OUTPUT_NAME "forma-lvgl-native"
    PREFIX ""
)

# Generate metadata hash
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/plugin.toml" TOML_CONTENT)
string(MD5 METADATA_HASH "${TOML_CONTENT}")

# Custom FNV-1a hash implementation
execute_process(
    COMMAND python3 -c "
import sys
def fnv1a_hash(data):
    hash_value = 0xcbf29ce484222325
    fnv_prime = 0x100000001b3
    for byte in data:
        hash_value ^= byte
        hash_value = (hash_value * fnv_prime) & 0xffffffffffffffff
    return hash_value

with open('${CMAKE_CURRENT_SOURCE_DIR}/plugin.toml', 'rb') as f:
    content = f.read()
    print(fnv1a_hash(content))
"
    OUTPUT_VARIABLE TOML_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add hash as compile definition
target_compile_definitions(forma_lvgl_native PRIVATE
    FORMA_PLUGIN_METADATA_HASH=${TOML_HASH}ULL
)

# Copy plugin.toml to output directory
add_custom_command(TARGET forma_lvgl_native POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin.toml
        $<TARGET_FILE_DIR:forma_lvgl_native>/plugin.toml
)
