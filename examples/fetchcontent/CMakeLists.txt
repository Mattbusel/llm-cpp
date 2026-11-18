# The integration in docs/INTEGRATION.md, as a project you can build:
#
#   cmake -S examples/fetchcontent -B build-fc && cmake --build build-fc
#
# CI builds it against the checkout instead of the release tarball with
#   -DFETCHCONTENT_SOURCE_DIR_LLM-CPP=<llm-cpp checkout>
cmake_minimum_required(VERSION 3.14)
project(llm_cpp_fetchcontent_example CXX)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)  # extracted files get the extraction time
endif()

include(FetchContent)
FetchContent_Declare(llm-cpp
  URL https://github.com/Mattbusel/llm-cpp/archive/refs/tags/v1.3.0.tar.gz)
FetchContent_MakeAvailable(llm-cpp)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE llm::cost llm::json)
