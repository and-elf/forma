# CMake Generator Plugin for Forma
cmake_minimum_required(VERSION 3.20)
project(FormaCMakeGenerator VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Forma core library (or use target if building together)
if(NOT TARGET forma_core)
    find_package(FormaCore REQUIRED)
    set(FORMA_CORE_TARGET FormaCore::forma_core)
else()
    set(FORMA_CORE_TARGET forma_core)
endif()

# CMake Generator Plugin
add_library(forma_cmake_generator SHARED
    src/cmake_generator.cpp
    src/cmake_generator.hpp
)

set_target_properties(forma_cmake_generator PROPERTIES
    PREFIX ""
    OUTPUT_NAME "forma-cmake-generator"
)

target_link_libraries(forma_cmake_generator PUBLIC ${FORMA_CORE_TARGET})

target_include_directories(forma_cmake_generator PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../src>
)

# Link with http-client and archive-utils plugins if needed for download support
option(CMAKE_GENERATOR_ENABLE_DOWNLOADS "Enable CMake auto-download support" OFF)

if(CMAKE_GENERATOR_ENABLE_DOWNLOADS)
    # Try to find the utility plugins
    find_package(FormaHTTPClient QUIET)
    find_package(FormaArchiveUtils QUIET)
    
    if(FormaHTTPClient_FOUND AND FormaArchiveUtils_FOUND)
        target_link_libraries(forma_cmake_generator PRIVATE 
            FormaPlugins::forma_http_client
            FormaPlugins::forma_archive_utils
        )
        target_compile_definitions(forma_cmake_generator PRIVATE FORMA_HAS_DOWNLOAD)
        message(STATUS "CMake generator: Download support enabled")
    else()
        message(STATUS "CMake generator: Download support requested but utility plugins not found")
        message(STATUS "  Build and install http-client and archive-utils plugins first")
    endif()
else()
    message(STATUS "CMake generator: Download support disabled")
endif()

target_include_directories(forma_cmake_generator PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/plugins/cmake-generator>
)

# Tests (plugin-specific option, independent of core FORMA_BUILD_TESTS)
option(CMAKE_GENERATOR_BUILD_TESTS "Build CMake generator plugin tests" OFF)
if(CMAKE_GENERATOR_BUILD_TESTS)
    # Add tests here when ready
    message(STATUS "CMake generator tests not yet implemented")
endif()

target_compile_definitions(forma_cmake_generator PRIVATE FORMA_PLUGIN_API=)

# Installation
install(TARGETS forma_cmake_generator
    EXPORT FormaCMakeGeneratorTargets
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
    INCLUDES DESTINATION include/forma/plugins/cmake-generator
)

install(FILES src/cmake_generator.hpp
    DESTINATION include/forma/plugins/cmake-generator
)

install(FILES README.md
    DESTINATION share/doc/forma-cmake-generator
)

# Export configuration
install(EXPORT FormaCMakeGeneratorTargets
    FILE FormaCMakeGeneratorTargets.cmake
    NAMESPACE FormaPlugins::
    DESTINATION lib/cmake/FormaCMakeGenerator
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCMakeGeneratorConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/FormaCMakeGeneratorConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCMakeGeneratorConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCMakeGeneratorConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FormaCMakeGeneratorConfigVersion.cmake"
    DESTINATION lib/cmake/FormaCMakeGenerator
)
