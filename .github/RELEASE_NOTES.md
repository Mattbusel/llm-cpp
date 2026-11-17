llm-cpp 1.2.1: fixes the MSVC build of `llm_stream.hpp` in 1.2.0. curl's headers include `windows.h`, whose `min` macro broke `std::min(...)` (error C2589) in every file that includes the header; it is now `(std::min)(...)`. 1.2.0 added OpenAI Responses API streaming (`llm::stream_openai_responses()`, usage and response id in `StreamStats`).

llm-cpp is an installable package: all 26 single-header C++17 libraries in `include/llm/`, with CMake targets, install rules and a package config.

## Use it from CMake

```cmake
include(FetchContent)
FetchContent_Declare(llm-cpp
  URL https://github.com/Mattbusel/llm-cpp/archive/refs/tags/v1.2.1.tar.gz)
FetchContent_MakeAvailable(llm-cpp)

target_link_libraries(app PRIVATE llm::cache llm::json llm::stream)
```

or install it (`cmake -S . -B build && cmake --install build`) and

```cmake
find_package(llm-cpp 1.2 CONFIG REQUIRED COMPONENTS cache json stream)
target_link_libraries(app PRIVATE llm::cache llm::json llm::stream)
```

In exactly one `.cpp` file, define each library's implementation macro before including it:

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include <llm/llm_cache.hpp>
```

## Targets

`llm::llm` (everything, plus `<llm/llm.hpp>`), and one per library: `llm::ab`, `llm::agent`, `llm::audio`, `llm::batch`, `llm::cache`, `llm::chat`, `llm::compress`, `llm::compress_summarize`, `llm::cost`, `llm::embed`, `llm::eval`, `llm::finetune`, `llm::format`, `llm::guard`, `llm::json`, `llm::log`, `llm::mock`, `llm::parse`, `llm::pool`, `llm::rag`, `llm::rank`, `llm::retry`, `llm::router`, `llm::stream`, `llm::template`, `llm::trace`, `llm::vision`.

## Dependencies

- libcurl (optional): ab, agent, audio, batch, chat, embed, eval, finetune, rag, rank, stream, vision and compress_summarize. Linked as `CURL::libcurl` when found; required when you ask for one of these components.
- Threads: batch, cache, finetune, mock, pool, retry, stream.
- Nothing else. The other 14 libraries are plain C++17.

## Tested

CI builds every header's compile-and-link test, all implementations in one binary, the offline examples (output diffed), llm-retry's regression tests and 106 upstream examples on Ubuntu (GCC, with and without libcurl), macOS (Apple Clang) and Windows (MSVC, libcurl from vcpkg), then builds a separate project against the installed package.

`include/llm/SOURCES.md` lists the source repository, commit and SHA-256 of every header. `llm-cpp-headers.zip` below has the headers in one flat folder for copy-paste use.

Full changes: [CHANGELOG.md](https://github.com/Mattbusel/llm-cpp/blob/v1.1.0/CHANGELOG.md)
