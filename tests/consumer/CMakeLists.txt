# A separate project that uses llm-cpp the way other projects will:
#   installed:      cmake -S tests/consumer -B b -DCMAKE_PREFIX_PATH=<install prefix>
#   build tree:     cmake -S tests/consumer -B b -Dllm-cpp_DIR=<llm-cpp build dir>
#   add_subdirectory / FetchContent:
#                   cmake -S tests/consumer -B b -DLLM_CPP_SOURCE_DIR=<llm-cpp checkout>
cmake_minimum_required(VERSION 3.14)
project(llm_cpp_consumer CXX)

option(CONSUMER_WITH_CURL "Also build against a libcurl component" OFF)

if(LLM_CPP_SOURCE_DIR)
    if(CONSUMER_WITH_CURL)
        find_package(CURL REQUIRED)
    endif()
    add_subdirectory("${LLM_CPP_SOURCE_DIR}" llm-cpp)
    message(STATUS "using llm-cpp from source")
elseif(CONSUMER_WITH_CURL)
    find_package(llm-cpp 1.0 CONFIG REQUIRED COMPONENTS cache json stream)
    message(STATUS "found llm-cpp ${llm-cpp_VERSION}")
else()
    find_package(llm-cpp 1.0 CONFIG REQUIRED COMPONENTS cache json)
    message(STATUS "found llm-cpp ${llm-cpp_VERSION}")
endif()

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE llm::cache llm::json)
if(CONSUMER_WITH_CURL)
    target_link_libraries(consumer PRIVATE llm::stream)
    target_compile_definitions(consumer PRIVATE CONSUMER_WITH_CURL)
endif()

# The umbrella target and header work too.
add_executable(consumer_umbrella umbrella.cpp)
target_link_libraries(consumer_umbrella PRIVATE llm::llm)

enable_testing()
add_test(NAME consumer COMMAND consumer)
add_test(NAME consumer_umbrella COMMAND consumer_umbrella)
