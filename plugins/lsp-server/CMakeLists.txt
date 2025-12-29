# LSP Server Plugin for Forma
cmake_minimum_required(VERSION 3.20)
project(FormaLSPServer VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Forma core library (or use target if building together)
if(NOT TARGET forma_core)
    find_package(FormaCore REQUIRED)
    set(FORMA_CORE_TARGET FormaCore::forma_core)
else()
    set(FORMA_CORE_TARGET forma_core)
endif()

# LSP library
add_library(forma_lsp SHARED
    src/lsp.hpp
    src/virtual_fs.hpp
    src/http_server.hpp
)

# Header-only library
set_target_properties(forma_lsp PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX ""
    OUTPUT_NAME "forma-lsp"
)

target_link_libraries(forma_lsp PUBLIC ${FORMA_CORE_TARGET})

target_include_directories(forma_lsp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/plugins/lsp-server>
)

# LSP Server executable (HTTP mode)
add_executable(forma_lsp_server
    src/lsp_server.cpp
)

target_link_libraries(forma_lsp_server PRIVATE forma_lsp)

# LSP Server executable (stdio mode for VSCode)
add_executable(forma_lsp_server_stdio
    src/lsp_server_stdio.cpp
)

target_link_libraries(forma_lsp_server_stdio PRIVATE forma_lsp)

# Tests (plugin-specific option, independent of core FORMA_BUILD_TESTS)
option(LSP_SERVER_BUILD_TESTS "Build LSP server plugin tests" OFF)
if(LSP_SERVER_BUILD_TESTS)
    # Find bugspray (should be available from parent project)
    if(NOT TARGET bugspray-with-main)
        message(FATAL_ERROR "bugspray-with-main target not found. Build forma with tests enabled.")
    endif()
    
    add_executable(forma_lsp_tests
        tests/lsp_tests.cpp
        tests/type_resolution_tests.cpp
        tests/vfs_tests.cpp
    )
    target_link_libraries(forma_lsp_tests PRIVATE forma_lsp bugspray-with-main)
    
    enable_testing()
    add_test(NAME forma_lsp_tests COMMAND forma_lsp_tests)
endif()

# Installation
install(TARGETS forma_lsp forma_lsp_server forma_lsp_server_stdio
    EXPORT FormaLSPServerTargets
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include/forma/plugins/lsp-server
)

install(FILES 
    src/lsp.hpp
    src/virtual_fs.hpp
    src/http_server.hpp
    DESTINATION include/forma/plugins/lsp-server
)

install(FILES README.md LSP_SERVER.md VIRTUAL_FS.md
    DESTINATION share/doc/forma-lsp-server
)

# Export configuration
install(EXPORT FormaLSPServerTargets
    FILE FormaLSPServerTargets.cmake
    NAMESPACE FormaPlugins::
    DESTINATION lib/cmake/FormaLSPServer
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLSPServerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/FormaLSPServerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLSPServerConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLSPServerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLSPServerConfigVersion.cmake"
    DESTINATION lib/cmake/FormaLSPServer
)
