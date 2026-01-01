cmake_minimum_required(VERSION 3.20)

project(FormaHTTPClient
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "HTTP/HTTPS download utility for Forma"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# CPM - CMake Package Manager
# ============================================================================

set(CPM_DOWNLOAD_VERSION 0.38.7)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
    )
endif()

include(${CPM_DOWNLOAD_LOCATION})

# ============================================================================
# Dependencies
# ============================================================================

# Archive utils plugin (for extract_archive)
# When building as part of the main Forma project, archive-utils is already added
# When building standalone, we need to add it ourselves
find_package(FormaArchiveUtils QUIET)
if(NOT FormaArchiveUtils_FOUND)
    # Check if we're building standalone or as part of main project
    if(NOT TARGET forma_archive_utils)
        # Only add subdirectory if the target doesn't exist yet
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../archive-utils/CMakeLists.txt")
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../archive-utils 
                             ${CMAKE_CURRENT_BINARY_DIR}/archive-utils EXCLUDE_FROM_ALL)
        else()
            message(FATAL_ERROR "FormaArchiveUtils plugin is required but not found. Build and install it first.")
        endif()
    endif()
    set(ARCHIVE_UTILS_TARGET forma_archive_utils)
else()
    set(ARCHIVE_UTILS_TARGET FormaPlugins::forma_archive_utils)
endif()

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

# ============================================================================
# HTTP Client Library
# ============================================================================

add_library(forma_http_client STATIC
    src/download.cpp
)

target_link_libraries(forma_http_client
    PUBLIC
        CURL::libcurl
        ${ARCHIVE_UTILS_TARGET}
)

target_include_directories(forma_http_client PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/http-client>
)

# Enable PIC for use in shared libraries (plugins)
set_target_properties(forma_http_client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# ============================================================================
# Installation (only when building standalone)
# ============================================================================

# Only install when building standalone (not as part of main Forma project)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    install(TARGETS forma_http_client
        EXPORT FormaHTTPClientTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/forma/http-client
    )

    install(FILES
        src/download.hpp
        DESTINATION include/forma/http-client
    )

    install(EXPORT FormaHTTPClientTargets
        FILE FormaHTTPClientTargets.cmake
        NAMESPACE FormaPlugins::
        DESTINATION lib/cmake/FormaHTTPClient
    )

    # Create package config
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/FormaHTTPClientConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/FormaHTTPClientConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/FormaHTTPClientConfig.cmake"
        INSTALL_DESTINATION lib/cmake/FormaHTTPClient
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/FormaHTTPClientConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/FormaHTTPClientConfigVersion.cmake"
        DESTINATION lib/cmake/FormaHTTPClient
    )
endif()
