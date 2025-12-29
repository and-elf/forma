# Forma Programming Language - Core Library
cmake_minimum_required(VERSION 3.20)

project(FormaCore 
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Forma - Core library with tokenizer, parser, and IR"
)

# C++20 required for constexpr features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(FORMA_BUILD_TESTS "Build compile-time tests" ON)
option(FORMA_BUILD_PLUGINS "Build bundled plugins" ON)
option(FORMA_ENABLE_DOWNLOADS "Enable HTTP/HTTPS download support (requires libcurl)" OFF)

# Compiler warnings - Always enabled, warnings are errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ============================================================================
# CPM - CMake Package Manager
# ============================================================================

include(cmake/CPM.cmake)

# ============================================================================
# External Dependencies
# ============================================================================

# Bugspray testing framework
add_subdirectory(external/bugspray EXCLUDE_FROM_ALL)

# Optional: libcurl for download support
if(FORMA_ENABLE_DOWNLOADS)
    message(STATUS "Download support enabled - fetching libcurl")
    
    # libcurl - HTTP/HTTPS downloads
    CPMAddPackage(
        NAME curl
        GITHUB_REPOSITORY curl/curl
        GIT_TAG curl-8_5_0
        OPTIONS
            "BUILD_CURL_EXE OFF"
            "BUILD_SHARED_LIBS OFF"
            "CURL_USE_OPENSSL ON"
            "HTTP_ONLY ON"
            "CURL_DISABLE_LDAP ON"
            "CURL_DISABLE_LDAPS ON"
            "CURL_DISABLE_TELNET ON"
            "CURL_DISABLE_DICT ON"
            "CURL_DISABLE_FILE ON"
            "CURL_DISABLE_TFTP ON"
            "CURL_DISABLE_RTSP ON"
            "CURL_DISABLE_POP3 ON"
            "CURL_DISABLE_IMAP ON"
            "CURL_DISABLE_SMTP ON"
            "CURL_DISABLE_GOPHER ON"
            "CURL_DISABLE_SMB ON"
            "CURL_USE_LIBSSH2 OFF"
            "CURL_USE_LIBPSL OFF"
            "ENABLE_MANUAL OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
    )
    
    # libarchive - Archive extraction
    CPMAddPackage(
        NAME libarchive
        GITHUB_REPOSITORY libarchive/libarchive
        GIT_TAG v3.7.2
        OPTIONS
            "ENABLE_TEST OFF"
            "ENABLE_TAR OFF"
            "ENABLE_CPIO OFF"
            "ENABLE_CAT OFF"
            "ENABLE_INSTALL OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "CMAKE_C_FLAGS -w"
            "CMAKE_CXX_FLAGS -w"
    )
endif()

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
# Archive Utility Library (Optional)
# ============================================================================
if(FORMA_ENABLE_DOWNLOADS)
    add_library(forma_archive STATIC
        src/core/archive.cpp
    )

    target_link_libraries(forma_archive
        PUBLIC
            archive_static
    )

    target_include_directories(forma_archive PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/forma>
    )
    
    # Add libarchive include directories
    target_include_directories(forma_archive PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/libarchive-src/libarchive
        ${CMAKE_BINARY_DIR}/_deps/libarchive-build
    )
    
    # Enable PIC for use in shared libraries (plugins)
    set_target_properties(forma_archive PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    
    message(STATUS "forma_archive library enabled")
endif()

# ============================================================================
# Download Utility Library (Optional)
# ============================================================================
if(FORMA_ENABLE_DOWNLOADS)
    add_library(forma_download STATIC
        src/core/download.cpp
    )

    target_link_libraries(forma_download
        PUBLIC
            CURL::libcurl
            forma_archive
    )

    target_include_directories(forma_download PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/forma>
    )
    
    # Enable PIC for use in shared libraries (plugins)
    set_target_properties(forma_download PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    
    message(STATUS "forma_download library enabled")
endif()

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
    add_subdirectory(plugins/deb-release)
endif()

# ============================================================================
# Demo Applications
# ============================================================================

option(FORMA_BUILD_DEMOS "Build demo applications" ON)
if(FORMA_BUILD_DEMOS)
    # Forma CLI Tool
    add_executable(forma forma.cpp)
    target_link_libraries(forma PRIVATE forma_core)
    
    # Conditionally link download support
    if(FORMA_ENABLE_DOWNLOADS)
        target_link_libraries(forma PRIVATE forma_download)
        target_compile_definitions(forma PRIVATE FORMA_HAS_DOWNLOAD)
    endif()
    
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
