# Forma Programming Language - Core Library
cmake_minimum_required(VERSION 3.20)

project(FormaCore 
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Forma - Core library with tokenizer, parser, and IR"
)

# C++20 required for constexpr features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(FORMA_BUILD_TESTS "Build compile-time tests" ON)
option(FORMA_BUILD_PLUGINS "Build bundled plugins" ON)
option(FORMA_ENABLE_COVERAGE "Enable code coverage analysis with gcovr" OFF)

# Compiler warnings - Always enabled, warnings are errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Coverage support
if(FORMA_ENABLE_COVERAGE)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Coverage analysis works best with Debug build type")
    endif()
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
        message(STATUS "Code coverage enabled (gcov/gcovr)")
    else()
        message(FATAL_ERROR "Coverage is only supported with GCC or Clang")
    endif()
endif()

# ============================================================================
# CPM - CMake Package Manager
# ============================================================================

include(cmake/CPM.cmake)

# ============================================================================
# External Dependencies
# ============================================================================

# CLI11 - Command-line parsing library
CPMAddPackage(
    NAME CLI11
    GITHUB_REPOSITORY CLIUtils/CLI11
    VERSION 2.4.1
    OPTIONS
        "CLI11_BUILD_DOCS OFF"
        "CLI11_BUILD_TESTS OFF"
        "CLI11_BUILD_EXAMPLES OFF"
)

# Bugspray testing framework
add_subdirectory(external/bugspray EXCLUDE_FROM_ALL)

# ============================================================================
# Core Library (Tokenizer, Parser, IR)
# ============================================================================

set(FORMA_CORE_HEADERS
    src/tokenizer/forma.hpp
    src/parser/ir.hpp
    src/core/export.hpp
    src/core/plugin.hpp
    src/toml/toml.hpp
)

# Core is header-only
add_library(forma_core INTERFACE)

target_include_directories(forma_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/tokenizer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/parser>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/toml>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
    $<INSTALL_INTERFACE:include/forma>
)

# ============================================================================
# Component Tests (Unit tests within each component)
# ============================================================================

if(FORMA_BUILD_TESTS)
    # Tokenizer tests (using bugspray)
    add_executable(forma_tokenizer_tests 
        src/tokenizer/tests/tokenizer_tests.cpp
    )
    target_link_libraries(forma_tokenizer_tests PRIVATE forma_core bugspray-with-main)
    target_compile_options(forma_tokenizer_tests PRIVATE -Wno-sign-compare)
    
    # Parser tests (using bugspray)
    add_executable(forma_parser_tests 
        src/parser/tests/parser_tests.cpp
    )
    target_link_libraries(forma_parser_tests PRIVATE forma_core bugspray-with-main)
    target_compile_options(forma_parser_tests PRIVATE -Wno-sign-compare)
    
    # Diagnostic tests (using bugspray)
    add_executable(forma_diagnostic_tests 
        src/parser/tests/diagnostic_tests.cpp
    )
    target_link_libraries(forma_diagnostic_tests PRIVATE forma_core bugspray-with-main)
    target_compile_options(forma_diagnostic_tests PRIVATE -Wno-sign-compare)
    
    # TOML tests (using bugspray)
    add_executable(forma_toml_tests 
        src/toml/tests/toml_tests.cpp
    )
    target_link_libraries(forma_toml_tests PRIVATE forma_core bugspray-with-main)
    target_compile_options(forma_toml_tests PRIVATE -Wno-sign-compare)
    
    # Enable testing
    enable_testing()
    add_test(NAME tokenizer_tests COMMAND forma_tokenizer_tests)
    add_test(NAME parser_tests COMMAND forma_parser_tests)
    add_test(NAME diagnostic_tests COMMAND forma_diagnostic_tests)
    add_test(NAME toml_tests COMMAND forma_toml_tests)
    
    add_custom_target(check 
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS forma_tokenizer_tests forma_parser_tests forma_diagnostic_tests forma_toml_tests
        COMMENT "Running all tests..."
    )
    
    # Coverage target (requires gcovr)
    if(FORMA_ENABLE_COVERAGE)
        find_program(GCOVR_EXECUTABLE gcovr)
        if(GCOVR_EXECUTABLE)
            add_custom_target(coverage
                # Run core tests only (not plugin tests which may not be built)
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "^(tokenizer|parser|diagnostic|toml)_tests$$"
                # Generate HTML coverage report
                COMMAND ${GCOVR_EXECUTABLE}
                    --root ${CMAKE_SOURCE_DIR}
                    --filter ${CMAKE_SOURCE_DIR}/src
                    --exclude ${CMAKE_SOURCE_DIR}/src/.*/tests/.*
                    --print-summary
                    --html-details ${CMAKE_BINARY_DIR}/coverage.html
                    --html-title "Forma Code Coverage"
                    --txt ${CMAKE_BINARY_DIR}/coverage.txt
                # Generate lcov format for VSCode Coverage Gutters extension
                COMMAND ${GCOVR_EXECUTABLE}
                    --root ${CMAKE_SOURCE_DIR}
                    --filter ${CMAKE_SOURCE_DIR}/src
                    --exclude ${CMAKE_SOURCE_DIR}/src/.*/tests/.*
                    --lcov ${CMAKE_BINARY_DIR}/coverage.lcov
                DEPENDS forma_tokenizer_tests forma_parser_tests forma_diagnostic_tests forma_toml_tests
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report with gcovr..."
                VERBATIM
            )
            message(STATUS "Coverage target available: make coverage")
            message(STATUS "  - HTML report: build/coverage.html")
            message(STATUS "  - lcov report: build/coverage.lcov (for VSCode)")
        else()
            message(WARNING "gcovr not found. Install with: pip install gcovr")
        endif()
    endif()
endif()

# ============================================================================
# Bundled Plugins (Optional - can be built separately)
# ============================================================================

if(FORMA_BUILD_PLUGINS)
    message(STATUS "Building bundled plugins...")
    
    # Export targets to build tree so plugins can find them
    export(TARGETS forma_core
        NAMESPACE FormaCore::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/FormaCoreTargets.cmake"
    )
    
    # Make FormaCore findable in build tree
    set(FormaCore_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    
    add_subdirectory(plugins/lvgl-renderer)
    add_subdirectory(plugins/lsp-server)
    add_subdirectory(plugins/cmake-generator)
    add_subdirectory(plugins/deb-deploy)
    add_subdirectory(plugins/esp32-lvgl)
endif()

# ============================================================================
# Demo Applications
# ============================================================================

option(FORMA_BUILD_DEMOS "Build demo applications" OFF)
if(FORMA_BUILD_DEMOS)
    # Forma CLI Tool
    add_executable(forma forma.cpp)
    target_link_libraries(forma PRIVATE forma_core CLI11::CLI11)
    target_compile_features(forma PRIVATE cxx_std_20)
    
    # Parser Demo
    add_executable(forma_parser_demo demos/parser_demo.cpp)
    target_link_libraries(forma_parser_demo PRIVATE forma_core)

    # Diagnostic Demo
    add_executable(forma_diagnostic_demo demos/diagnostic_demo.cpp)
    target_link_libraries(forma_diagnostic_demo PRIVATE forma_core)
endif()

# ============================================================================
# Installation
# ============================================================================

# Create FormaCore package
install(TARGETS forma_core
    EXPORT FormaCoreTargets
    INCLUDES DESTINATION include/forma
)

# Install headers in their component directories
install(DIRECTORY src/tokenizer/ 
    DESTINATION include/forma/tokenizer
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "tests" EXCLUDE
)

install(DIRECTORY src/parser/
    DESTINATION include/forma/parser
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "tests" EXCLUDE
)

install(DIRECTORY src/toml/
    DESTINATION include/forma/toml
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "tests" EXCLUDE
)

install(DIRECTORY src/core/
    DESTINATION include/forma/core
    FILES_MATCHING PATTERN "*.hpp"
)

# Export FormaCore targets
install(EXPORT FormaCoreTargets
    FILE FormaCoreTargets.cmake
    NAMESPACE FormaCore::
    DESTINATION lib/cmake/FormaCore
)

# Create package config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/FormaCoreConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCoreConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCoreConfigVersion.cmake"
    DESTINATION lib/cmake/FormaCore
)

# Install documentation
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION share/doc/forma
    FILES_MATCHING 
    PATTERN "*.md"
    PATTERN "*.toml"
    PATTERN "build/" EXCLUDE
    PATTERN ".git/" EXCLUDE
    PATTERN "plugins/" EXCLUDE
)

# ============================================================================
# Development helpers
# ============================================================================

# Print configuration summary
message(STATUS "")
message(STATUS "Forma Core Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${FORMA_BUILD_TESTS}")
message(STATUS "  Build Plugins: ${FORMA_BUILD_PLUGINS}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Forma is a core library providing:")
message(STATUS "  - Tokenizer and lexer")
message(STATUS "  - Parser and IR structures")
message(STATUS "  - Plugin interface")
message(STATUS "")
message(STATUS "Plugins are in plugins/ directory and can be built separately")
message(STATUS "")
