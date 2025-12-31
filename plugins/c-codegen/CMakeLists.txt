# C Code Generator Plugin for Forma
cmake_minimum_required(VERSION 3.20)
project(FormaCCodegen VERSION 1.0.0 LANGUAGES CXX)

# C++23 required for constexpr features
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
add_library(forma_c_codegen SHARED
    src/c_codegen.hpp
    src/c_codegen_plugin.cpp
)

# Plugin library configuration
set_target_properties(forma_c_codegen PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX ""
    OUTPUT_NAME "forma-c-codegen"
)

target_include_directories(forma_c_codegen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/parser
)

target_link_libraries(forma_c_codegen PRIVATE ${FORMA_CORE_TARGET})

# Tests
option(C_CODEGEN_BUILD_TESTS "Build C codegen plugin tests" ON)
if(C_CODEGEN_BUILD_TESTS)
    # Use bugspray from parent project
    if(NOT TARGET bugspray-with-main)
        message(WARNING "bugspray-with-main target not found. Skipping C codegen tests.")
    else()
        add_executable(c_codegen_tests
            tests/c_codegen_tests.cpp
        )
        target_include_directories(c_codegen_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src
            ${CMAKE_CURRENT_SOURCE_DIR}/../../src/parser
        )
        target_link_libraries(c_codegen_tests PRIVATE forma_c_codegen bugspray-with-main)
        
        enable_testing()
        add_test(NAME c_codegen_tests COMMAND c_codegen_tests)
    endif()
endif()

# Installation
install(TARGETS forma_c_codegen
    LIBRARY DESTINATION lib/forma/plugins
    RUNTIME DESTINATION lib/forma/plugins
)

install(FILES src/c_codegen.hpp
    DESTINATION include/forma/plugins/c-codegen
)
