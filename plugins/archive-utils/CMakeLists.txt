cmake_minimum_required(VERSION 3.20)

project(FormaArchiveUtils
    VERSION 0.1.0
    LANGUAGES CXX C
    DESCRIPTION "Archive extraction utility for Forma"
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

# ============================================================================
# Archive Utils Library
# ============================================================================

if(NOT TARGET forma_archive_utils)
    add_library(forma_archive_utils STATIC
        src/archive.cpp
    )

    target_link_libraries(forma_archive_utils
        PRIVATE
            archive_static
    )

    target_include_directories(forma_archive_utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/forma/archive-utils>
    )

    # Add libarchive include directories
    target_include_directories(forma_archive_utils PRIVATE
        ${CMAKE_BINARY_DIR}/_deps/libarchive-src/libarchive
        ${CMAKE_BINARY_DIR}/_deps/libarchive-build
    )

    # Enable PIC for use in shared libraries (plugins)
    set_target_properties(forma_archive_utils PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# ============================================================================
# Installation (only when building standalone)
# ============================================================================

# Only install when building standalone (not as part of main Forma project)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    install(TARGETS forma_archive_utils
        EXPORT FormaArchiveUtilsTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(FILES
        src/archive.hpp
        DESTINATION include/forma/archive-utils
    )

    # Export targets
    install(EXPORT FormaArchiveUtilsTargets
        FILE FormaArchiveUtilsTargets.cmake
        NAMESPACE FormaPlugins::
        DESTINATION lib/cmake/FormaArchiveUtils
    )

    # Create package config files
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/FormaArchiveUtilsConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    # Simple config file that includes the targets
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/FormaArchiveUtilsConfig.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/FormaArchiveUtilsTargets.cmake\")
")

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/FormaArchiveUtilsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/FormaArchiveUtilsConfigVersion.cmake"
        DESTINATION lib/cmake/FormaArchiveUtils
    )
endif()
