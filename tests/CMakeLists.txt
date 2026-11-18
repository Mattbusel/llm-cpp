# llm-cpp tests
#
#   header_<name>   one per header: the implementation TU (headers/<name>.cpp)
#                   plus a second TU that includes the header again; must link
#                   and run
#   link_all        every implementation in one binary + <llm/llm.hpp>
#   hygiene         every header with NOMINMAX predefined; LLM_TRACE twice
#                   in one scope
#   windows_h_<name>
#                   (Windows) compile-only: the header after <windows.h>
#                   without NOMINMAX, whose min/max macros break std::min(...)
#   offline_<name>  examples/offline/<name>.cpp, output diffed against
#                   examples/offline/output/<name>.txt
#   upstream_<name> a library's own test program (tests/upstream/<name>/)
#   example_<name>_<file>
#                   every upstream example (tests/examples/<name>/), compiled
#                   and linked only: they call real APIs and need keys
#
# Components that need libcurl are skipped when CURL::libcurl is not found.

set(_llm_have_curl OFF)
if(TARGET CURL::libcurl)
    set(_llm_have_curl ON)
endif()

function(_llm_usable out name)
    if(name IN_LIST LLM_CPP_CURL_COMPONENTS AND NOT _llm_have_curl)
        set(${out} OFF PARENT_SCOPE)
    else()
        set(${out} ON PARENT_SCOPE)
    endif()
endfunction()

function(_llm_warnings tgt)
    if(MSVC)
        target_compile_options(${tgt} PRIVATE /W3 /utf-8)
        target_compile_definitions(${tgt} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${tgt} PRIVATE -Wall)
    endif()
endfunction()

# The header tests and link_all hold the library code itself. With
# LLM_CPP_STRICT_WARNINGS they build at the warning level of a strict
# consuming project, as errors, so a warning in a header fails CI here
# instead of breaking someone else's -Werror build.
function(_llm_strict tgt)
    if(NOT LLM_CPP_STRICT_WARNINGS)
        _llm_warnings(${tgt})
    elseif(MSVC)
        target_compile_options(${tgt} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
        target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
    endif()
endfunction()

set(_llm_skipped)
set(_llm_impl_sources)
set(_llm_all_libs)

foreach(name IN LISTS LLM_CPP_COMPONENTS)
    _llm_usable(ok ${name})
    if(NOT ok)
        list(APPEND _llm_skipped ${name})
        continue()
    endif()
    set(LLM_NAME ${name})
    configure_file(second_tu.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/second_tu_${name}.cpp" @ONLY)
    add_executable(header_${name}
        headers/${name}.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/second_tu_${name}.cpp")
    target_link_libraries(header_${name} PRIVATE llm::${name})
    _llm_strict(header_${name})
    add_test(NAME header_${name} COMMAND header_${name})

    if(WIN32)
        string(TOUPPER ${name} LLM_NAME_UPPER)
        configure_file(windows_h.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/windows_h_${name}.cpp" @ONLY)
        add_library(windows_h_${name} OBJECT "${CMAKE_CURRENT_BINARY_DIR}/windows_h_${name}.cpp")
        target_link_libraries(windows_h_${name} PRIVATE llm::${name})
        _llm_strict(windows_h_${name})
    endif()

    list(APPEND _llm_impl_sources headers/${name}.cpp)
    list(APPEND _llm_all_libs llm::${name})
endforeach()

if(_llm_skipped)
    message(STATUS "llm-cpp tests: no libcurl, skipping: ${_llm_skipped}")
endif()

add_executable(link_all link_all.cpp ${_llm_impl_sources})
target_link_libraries(link_all PRIVATE llm::llm ${_llm_all_libs})
target_compile_definitions(link_all PRIVATE LLM_CPP_EXPECTED_VERSION="${PROJECT_VERSION}")
_llm_strict(link_all)
add_test(NAME link_all COMMAND link_all)

# Header hygiene: NOMINMAX defined on the command line, as many Windows
# projects do (no header may redefine it), and two LLM_TRACE spans in one scope.
add_executable(hygiene hygiene.cpp)
target_link_libraries(hygiene PRIVATE llm::llm)
target_compile_definitions(hygiene PRIVATE NOMINMAX)
_llm_strict(hygiene)
add_test(NAME hygiene COMMAND hygiene)

# llm_compress.hpp's summarize path (LLM_COMPRESS_SUMMARIZE, libcurl).
if(_llm_have_curl)
    add_executable(header_compress_summarize
        headers/compress.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/second_tu_compress.cpp")
    target_link_libraries(header_compress_summarize PRIVATE llm::compress_summarize)
    _llm_strict(header_compress_summarize)
    add_test(NAME header_compress_summarize COMMAND header_compress_summarize)
endif()

# Offline examples whose printed output is committed in the repo.
file(GLOB _llm_offline RELATIVE "${PROJECT_SOURCE_DIR}/examples/offline"
     "${PROJECT_SOURCE_DIR}/examples/offline/*.cpp")
foreach(src IN LISTS _llm_offline)
    get_filename_component(name ${src} NAME_WE)
    add_executable(offline_${name} "${PROJECT_SOURCE_DIR}/examples/offline/${src}")
    target_link_libraries(offline_${name} PRIVATE llm::${name})
    _llm_warnings(offline_${name})
    add_test(NAME offline_${name}
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:offline_${name}>
            -DEXPECTED=${PROJECT_SOURCE_DIR}/examples/offline/output/${name}.txt
            -P ${CMAKE_CURRENT_SOURCE_DIR}/compare_output.cmake)
endforeach()

# A library's own tests, run as-is.
file(GLOB _llm_upstream RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/upstream"
     "${CMAKE_CURRENT_SOURCE_DIR}/upstream/*/*.cpp")
foreach(src IN LISTS _llm_upstream)
    get_filename_component(name ${src} DIRECTORY)
    get_filename_component(file ${src} NAME_WE)
    _llm_usable(ok ${name})
    if(NOT ok)
        continue()
    endif()
    add_executable(upstream_${name}_${file} upstream/${src})
    target_link_libraries(upstream_${name}_${file} PRIVATE llm::${name})
    _llm_warnings(upstream_${name}_${file})
    add_test(NAME upstream_${name}_${file} COMMAND upstream_${name}_${file})
endforeach()

# Every upstream example: compile and link (no run, they need API keys).
# Each example links the targets of every llm_*.hpp it includes.
#
# Known-broken upstream examples, skipped until fixed in their repository:
#   cache/cached_openai  when llm_stream.hpp is on the include path it uses an
#                        older llm-cache API (llm::Cache, llm::cached_call,
#                        CacheConfig::cache_dir) that the header no longer has
set(LLM_CPP_SKIP_EXAMPLES cache/cached_openai)
if(LLM_CPP_BUILD_EXAMPLES)
    file(GLOB _llm_examples RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/examples"
         "${CMAKE_CURRENT_SOURCE_DIR}/examples/*/*.cpp")
    set(_llm_example_count 0)
    foreach(src IN LISTS _llm_examples)
        get_filename_component(name ${src} DIRECTORY)
        get_filename_component(file ${src} NAME_WE)
        if("${name}/${file}" IN_LIST LLM_CPP_SKIP_EXAMPLES)
            continue()
        endif()
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/examples/${src}" _incs
             REGEX "#[ \t]*include[ \t]*[<\"](llm/)?llm_[a-z_]+\.hpp")
        set(_libs ${name})
        foreach(line IN LISTS _incs)
            string(REGEX MATCH "llm_([a-z_]+)\.hpp" _m "${line}")
            list(APPEND _libs ${CMAKE_MATCH_1})
        endforeach()
        list(REMOVE_DUPLICATES _libs)
        set(ok ON)
        foreach(l IN LISTS _libs)
            _llm_usable(_u ${l})
            if(NOT _u)
                set(ok OFF)
            endif()
        endforeach()
        if(NOT ok)
            continue()
        endif()
        set(tgt example_${name}_${file})
        add_executable(${tgt} examples/${src})
        foreach(l IN LISTS _libs)
            target_link_libraries(${tgt} PRIVATE llm::${l})
        endforeach()
        _llm_warnings(${tgt})
        math(EXPR _llm_example_count "${_llm_example_count} + 1")
    endforeach()
    message(STATUS "llm-cpp tests: building ${_llm_example_count} upstream examples")
endif()
