cmake_minimum_required(VERSION 3.14)

# The version lives in include/llm/llm.hpp so the header and the package agree.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/llm/llm.hpp" _llm_version_line
     REGEX "^#define LLM_CPP_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\"")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" _llm_version "${_llm_version_line}")

project(llm-cpp
    VERSION ${_llm_version}
    DESCRIPTION "26 single-header C++17 libraries for LLM features: streaming, retries, caching, cost, RAG, agents and more"
    HOMEPAGE_URL "https://github.com/Mattbusel/llm-cpp"
    LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_llm_is_top ON)
else()
    set(_llm_is_top OFF)
endif()

option(LLM_CPP_BUILD_TESTS    "Build the llm-cpp compile and link tests" ${_llm_is_top})
option(LLM_CPP_BUILD_EXAMPLES "Build every library's upstream examples as part of the tests" ON)
option(LLM_CPP_INSTALL        "Generate the install target" ${_llm_is_top})
option(LLM_CPP_STRICT_WARNINGS "Build the header tests with -Wall -Wextra -Wpedantic -Wshadow (/W4 on MSVC) as errors" OFF)

# ---------------------------------------------------------------------------
# Components. Every library is a header-only INTERFACE target llm::<name>.
#
#   LLM_CPP_PLAIN_COMPONENTS  no external dependency
#   LLM_CPP_CURL_COMPONENTS   implementation (and for eval/stream, the header
#                             itself) needs libcurl
#   LLM_CPP_THREAD_COMPONENTS use std::thread / std::mutex and link Threads
#
# This list is mirrored in cmake/llm-cppConfig.cmake.in.
# ---------------------------------------------------------------------------
set(LLM_CPP_PLAIN_COMPONENTS
    cache compress cost format guard json log mock parse pool retry router template trace)
set(LLM_CPP_CURL_COMPONENTS
    ab agent audio batch chat embed eval finetune rag rank stream vision)
set(LLM_CPP_THREAD_COMPONENTS
    batch cache finetune mock pool retry stream)
set(LLM_CPP_COMPONENTS ${LLM_CPP_PLAIN_COMPONENTS} ${LLM_CPP_CURL_COMPONENTS})
list(SORT LLM_CPP_COMPONENTS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# libcurl is optional: the targets that need it link CURL::libcurl when it is
# found (here, or later by the consuming project before it links them).
find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "llm-cpp: libcurl found, network components are fully usable")
else()
    message(STATUS "llm-cpp: libcurl not found; network components (${LLM_CPP_CURL_COMPONENTS}) will need CURL::libcurl from the consuming project")
endif()

set(_llm_inc_build   "${PROJECT_SOURCE_DIR}/include")
set(_llm_targets)

function(_llm_add_component name)
    set(tgt llm_${name})
    add_library(${tgt} INTERFACE)
    add_library(llm::${name} ALIAS ${tgt})
    set_target_properties(${tgt} PROPERTIES EXPORT_NAME ${name})
    target_compile_features(${tgt} INTERFACE cxx_std_17)
    # Both spellings work: #include <llm/llm_cache.hpp> and #include "llm_cache.hpp".
    target_include_directories(${tgt} INTERFACE
        $<BUILD_INTERFACE:${_llm_inc_build}>
        $<BUILD_INTERFACE:${_llm_inc_build}/llm>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/llm>)
    if(name IN_LIST LLM_CPP_THREAD_COMPONENTS)
        target_link_libraries(${tgt} INTERFACE Threads::Threads)
    endif()
    if(name IN_LIST LLM_CPP_CURL_COMPONENTS)
        target_link_libraries(${tgt} INTERFACE $<TARGET_NAME_IF_EXISTS:CURL::libcurl>)
    endif()
    set(_llm_targets ${_llm_targets} ${tgt} PARENT_SCOPE)
endfunction()

foreach(c IN LISTS LLM_CPP_COMPONENTS)
    _llm_add_component(${c})
endforeach()

# llm::compress_summarize: llm::compress with LLM_COMPRESS_SUMMARIZE defined,
# which turns on summarize-based compression over libcurl.
add_library(llm_compress_summarize INTERFACE)
add_library(llm::compress_summarize ALIAS llm_compress_summarize)
set_target_properties(llm_compress_summarize PROPERTIES EXPORT_NAME compress_summarize)
target_compile_definitions(llm_compress_summarize INTERFACE LLM_COMPRESS_SUMMARIZE)
target_link_libraries(llm_compress_summarize INTERFACE llm_compress $<TARGET_NAME_IF_EXISTS:CURL::libcurl>)
list(APPEND _llm_targets llm_compress_summarize)

# llm::llm: every library, plus the umbrella header <llm/llm.hpp>.
add_library(llm_llm INTERFACE)
add_library(llm::llm ALIAS llm_llm)
set_target_properties(llm_llm PROPERTIES EXPORT_NAME llm)
foreach(c IN LISTS LLM_CPP_COMPONENTS)
    target_link_libraries(llm_llm INTERFACE llm_${c})
endforeach()
list(APPEND _llm_targets llm_llm)

# ---------------------------------------------------------------------------
# Install + package config: find_package(llm-cpp CONFIG [COMPONENTS ...])
# ---------------------------------------------------------------------------
if(LLM_CPP_INSTALL)
    set(LLM_CPP_CMAKE_DIR "${CMAKE_INSTALL_DATADIR}/llm-cpp" CACHE STRING
        "Install directory for llm-cppConfig.cmake, relative to the prefix")

    install(TARGETS ${_llm_targets} EXPORT llm-cppTargets)
    install(DIRECTORY include/llm DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "SOURCES.md")
    install(FILES LICENSE CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

    install(EXPORT llm-cppTargets
            NAMESPACE llm::
            DESTINATION ${LLM_CPP_CMAKE_DIR})

    configure_package_config_file(
        cmake/llm-cppConfig.cmake.in
        "${PROJECT_BINARY_DIR}/llm-cppConfig.cmake"
        INSTALL_DESTINATION ${LLM_CPP_CMAKE_DIR})
    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/llm-cppConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)
    install(FILES
        "${PROJECT_BINARY_DIR}/llm-cppConfig.cmake"
        "${PROJECT_BINARY_DIR}/llm-cppConfigVersion.cmake"
        DESTINATION ${LLM_CPP_CMAKE_DIR})

    # pkg-config: header-only. When libcurl was found, the network headers'
    # dependency is declared so `pkg-config --cflags --libs llm-cpp` brings it.
    if(CURL_FOUND)
        set(LLM_CPP_PC_REQUIRES "Requires: libcurl")
    else()
        set(LLM_CPP_PC_REQUIRES "")
    endif()
    configure_file(cmake/llm-cpp.pc.in "${PROJECT_BINARY_DIR}/llm-cpp.pc" @ONLY)
    install(FILES "${PROJECT_BINARY_DIR}/llm-cpp.pc"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)

    # Lets another project use this build tree directly (llm-cpp_DIR=<build>).
    export(EXPORT llm-cppTargets NAMESPACE llm::
           FILE "${PROJECT_BINARY_DIR}/llm-cppTargets.cmake")
endif()

if(LLM_CPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
