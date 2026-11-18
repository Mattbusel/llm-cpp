llm-cpp 1.4.0: `llm_stream.hpp` gains a standalone Server-Sent Events parser. `llm::SseParser` (incremental) and `llm::parse_sse(body)` follow the WHATWG event-stream rules (CRLF/CR line ends split across chunks, comments, multi-line `data:`, `id:`, `retry:`), are header-only with no `LLM_STREAM_IMPLEMENTATION`, and with `LLM_STREAM_NO_CURL` compile without libcurl, so projects with their own HTTP client can use them. The libcurl streaming functions now run on the same parser. See [CHANGELOG.md](https://github.com/Mattbusel/llm-cpp/blob/v1.4.0/CHANGELOG.md).

llm-cpp is an installable package: all 26 single-header C++17 libraries in `include/llm/`, with CMake targets, install rules and a package config.

## Use it from CMake

```cmake
include(FetchContent)
FetchContent_Declare(llm-cpp
  URL https://github.com/Mattbusel/llm-cpp/archive/refs/tags/v1.4.0.tar.gz)
FetchContent_MakeAvailable(llm-cpp)

target_link_libraries(app PRIVATE llm::cache llm::json llm::stream)
```

or install it (`cmake -S . -B build && cmake --install build`) and

```cmake
find_package(llm-cpp 1.3 CONFIG REQUIRED COMPONENTS cache json stream)
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

CI builds every header's compile-and-link test, all implementations in one binary, the offline examples (output diffed), llm-retry's regression tests and 106 upstream examples on Ubuntu (GCC with and without libcurl, Clang), macOS (Apple Clang) and Windows (MSVC, libcurl from vcpkg), with the header tests at `-Werror` / `/WX`, then builds a separate project against the installed package and the FetchContent example.

`include/llm/SOURCES.md` lists the source repository, commit and SHA-256 of every header. `llm-cpp-headers.zip` below has the headers in one flat folder for copy-paste use.

Full changes: [CHANGELOG.md](https://github.com/Mattbusel/llm-cpp/blob/v1.3.0/CHANGELOG.md)
