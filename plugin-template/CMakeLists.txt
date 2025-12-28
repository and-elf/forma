# Forma Example Plugin
cmake_minimum_required(VERSION 3.20)
project(FormaExamplePlugin VERSION 1.0.0 LANGUAGES CXX)

# C++20 required for Forma
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Forma core library
# Users should set: -DFormaCore_DIR=/path/to/forma/lib/cmake/FormaCore
find_package(FormaCore REQUIRED)

# Plugin library
add_library(forma_example_plugin SHARED
    src/example_plugin.hpp
    src/example_plugin.cpp
)

set_target_properties(forma_example_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "forma-example-plugin"
)

target_link_libraries(forma_example_plugin PUBLIC FormaCore::forma_core)

target_include_directories(forma_example_plugin PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/plugins/example-plugin>
)

# Tests
option(BUILD_TESTS "Build plugin tests" ON)
if(BUILD_TESTS)
    add_executable(example_plugin_tests
        tests/example_plugin_tests.cpp
    )
    target_link_libraries(example_plugin_tests PRIVATE forma_example_plugin)
    
    enable_testing()
    add_test(NAME example_plugin_tests COMMAND example_plugin_tests)
endif()

# Examples
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_executable(example_demo
        examples/example.cpp
    )
    target_link_libraries(example_demo PRIVATE forma_example_plugin)
endif()

# Installation
install(TARGETS forma_example_plugin
    EXPORT FormaExamplePluginTargets
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
    INCLUDES DESTINATION include/forma/plugins/example-plugin
)

install(FILES src/example_plugin.hpp
    DESTINATION include/forma/plugins/example-plugin
)

install(FILES README.md
    DESTINATION share/doc/forma-example-plugin
)

# Export configuration
install(EXPORT FormaExamplePluginTargets
    FILE FormaExamplePluginTargets.cmake
    NAMESPACE FormaPlugins::
    DESTINATION lib/cmake/FormaExamplePlugin
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FormaExamplePluginConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/FormaExamplePluginConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/FormaExamplePluginConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FormaExamplePluginConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FormaExamplePluginConfigVersion.cmake"
    DESTINATION lib/cmake/FormaExamplePlugin
)
