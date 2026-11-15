llm-cpp 1.1.0: `llm_cost.hpp` prices 131 models from 10 providers (OpenAI, Anthropic, Google, DeepSeek, Mistral, xAI, Cohere, Groq, Together, Fireworks) instead of 12. The table is generated from [llm-prices](https://github.com/Mattbusel/llm-prices) by `tools/gen_cost_table.py`, and a weekly job fails when llm-prices changes a price. New in llm-cost: `find_model("deepseek-chat")`, `all_models()` and `compare_costs(text, models)`. The twelve original constants are unchanged.

llm-cpp is an installable package: all 26 single-header C++17 libraries in `include/llm/`, with CMake targets, install rules and a package config.

## Use it from CMake

```cmake
include(FetchContent)
FetchContent_Declare(llm-cpp
  URL https://github.com/Mattbusel/llm-cpp/archive/refs/tags/v1.1.0.tar.gz)
FetchContent_MakeAvailable(llm-cpp)

target_link_libraries(app PRIVATE llm::cache llm::json llm::stream)
```

or install it (`cmake -S . -B build && cmake --install build`) and

```cmake
find_package(llm-cpp 1.1 CONFIG REQUIRED COMPONENTS cache json stream)
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
