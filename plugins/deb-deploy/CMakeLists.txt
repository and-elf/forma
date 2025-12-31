# Debian Release Plugin for Forma
# Generates Debian package metadata and control files

cmake_minimum_required(VERSION 3.20)
project(FormaDebDeployPlugin VERSION 0.1.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Plugin library
add_library(forma_deb_deploy SHARED
    src/deb_deploy.hpp
    src/deb_deploy_plugin.cpp
)

set_target_properties(forma_deb_deploy PROPERTIES
    PREFIX ""
    OUTPUT_NAME "forma-deb-deploy"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(forma_deb_deploy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../src>
    $<INSTALL_INTERFACE:include/forma/plugins/deb-deploy>
)

# Tests
option(DEB_DEPLOY_BUILD_TESTS "Build Debian release plugin tests" ON)
if(DEB_DEPLOY_BUILD_TESTS)
    # Use bugspray from parent project
    if(NOT TARGET bugspray-with-main)
        message(WARNING "bugspray-with-main target not found. Skipping deb-deploy tests.")
    else()
        add_executable(deb_deploy_tests
            tests/deb_deploy_tests.cpp
        )
        target_link_libraries(deb_deploy_tests PRIVATE forma_deb_deploy bugspray-with-main)
        
        enable_testing()
        add_test(NAME deb_deploy_tests COMMAND deb_deploy_tests)
    endif()
endif()

# Installation
install(TARGETS forma_deb_deploy
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
)

install(FILES src/deb_deploy.hpp
    DESTINATION include/forma/plugins/deb-deploy
)
