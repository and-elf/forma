# Tracer Plugin for Forma Compiler
# Provides logging and diagnostic capabilities

cmake_minimum_required(VERSION 3.15)
project(FormaTracerPlugin VERSION 0.1.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library
add_library(FormaTracer INTERFACE)

target_include_directories(FormaTracer INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Installation
install(TARGETS FormaTracer
    EXPORT FormaTracerTargets
    INCLUDES DESTINATION include
)

install(FILES src/tracer_plugin.hpp
    DESTINATION include/forma/tracer
)

install(EXPORT FormaTracerTargets
    FILE FormaTracerConfig.cmake
    NAMESPACE Forma::
    DESTINATION lib/cmake/FormaTracer
)

# Package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FormaTracerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/FormaTracerConfigVersion.cmake"
    DESTINATION lib/cmake/FormaTracer
)

# Tests
option(TRACER_BUILD_TESTS "Build tracer plugin tests" ON)
if(TRACER_BUILD_TESTS)
    # Use bugspray from parent project
    if(NOT TARGET bugspray-with-main)
        message(WARNING "bugspray-with-main target not found. Skipping tracer tests.")
    else()
        add_executable(tracer_tests
            tests/test_tracer.cpp
        )
        target_link_libraries(tracer_tests PRIVATE FormaTracer bugspray-with-main)
        
        enable_testing()
        add_test(NAME tracer_tests COMMAND tracer_tests)
    endif()
endif()
