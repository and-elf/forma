# LVGL Renderer Plugin for Forma
cmake_minimum_required(VERSION 3.20)
project(FormaLVGLRenderer VERSION 1.0.0 LANGUAGES CXX)

# C++20 required for constexpr features
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

# Plugin library
add_library(forma_lvgl_renderer SHARED
    src/lvgl_renderer.hpp
    src/lvgl_renderer_plugin.cpp
)

# Plugin library configuration
set_target_properties(forma_lvgl_renderer PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX ""
    OUTPUT_NAME "forma-lvgl-renderer"
)

target_link_libraries(forma_lvgl_renderer PUBLIC ${FORMA_CORE_TARGET})

target_include_directories(forma_lvgl_renderer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/forma/plugins/lvgl-renderer>
)

target_include_directories(forma_lvgl_renderer PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../src>
)

# Tests (plugin-specific option, independent of core FORMA_BUILD_TESTS)
option(LVGL_RENDERER_BUILD_TESTS "Build LVGL renderer plugin tests" ON)
if(LVGL_RENDERER_BUILD_TESTS)
    # Use bugspray from parent project
    if(NOT TARGET bugspray-with-main)
        message(WARNING "bugspray-with-main target not found. Skipping LVGL renderer tests.")
    else()
        add_executable(lvgl_renderer_tests
            tests/lvgl_tests.cpp
        )
        target_link_libraries(lvgl_renderer_tests PRIVATE forma_lvgl_renderer bugspray-with-main)
        
        add_executable(lvgl_renderer_demo
            tests/lvgl_demo.cpp
        )
        target_link_libraries(lvgl_renderer_demo PRIVATE forma_lvgl_renderer)
        
        enable_testing()
        add_test(NAME lvgl_renderer_tests COMMAND lvgl_renderer_tests)
    endif()
endif()

# Installation
install(TARGETS forma_lvgl_renderer
    EXPORT FormaLVGLRendererTargets
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
    INCLUDES DESTINATION include/forma/plugins/lvgl-renderer
)

install(FILES src/lvgl_renderer.hpp
    DESTINATION include/forma/plugins/lvgl-renderer
)

install(FILES README.md
    DESTINATION share/doc/forma-lvgl-renderer
)

# Export configuration
install(EXPORT FormaLVGLRendererTargets
    FILE FormaLVGLRendererTargets.cmake
    NAMESPACE FormaPlugins::
    DESTINATION lib/cmake/FormaLVGLRenderer
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLVGLRendererConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/FormaLVGLRendererConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLVGLRendererConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLVGLRendererConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FormaLVGLRendererConfigVersion.cmake"
    DESTINATION lib/cmake/FormaLVGLRenderer
)
