# Debian Release Plugin for Forma
# Generates Debian package metadata and control files

cmake_minimum_required(VERSION 3.20)
project(FormaDebReleasePlugin VERSION 0.1.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Plugin library
add_library(forma_deb_release SHARED
    src/deb_release.hpp
    src/deb_release_plugin.cpp
)

set_target_properties(forma_deb_release PROPERTIES
    PREFIX ""
    OUTPUT_NAME "forma-deb-release"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(forma_deb_release PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/plugins/deb-release>
)

# Tests
option(DEB_RELEASE_BUILD_TESTS "Build Debian release plugin tests" ON)
if(DEB_RELEASE_BUILD_TESTS)
    # Use bugspray from parent project
    if(NOT TARGET bugspray-with-main)
        message(WARNING "bugspray-with-main target not found. Skipping deb-release tests.")
    else()
        add_executable(deb_release_tests
            tests/deb_release_tests.cpp
        )
        target_link_libraries(deb_release_tests PRIVATE forma_deb_release bugspray-with-main)
        
        enable_testing()
        add_test(NAME deb_release_tests COMMAND deb_release_tests)
    endif()
endif()

# Installation
install(TARGETS forma_deb_release
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
)

install(FILES src/deb_release.hpp
    DESTINATION include/forma/plugins/deb-release
)
