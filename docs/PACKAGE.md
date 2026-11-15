# Using llm-cpp as a package

llm-cpp 1.x ships all 26 headers in `include/llm/` with CMake targets, so you can
depend on it instead of copying files.

## FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(llm-cpp
  URL https://github.com/Mattbusel/llm-cpp/archive/refs/tags/v1.1.0.tar.gz)
FetchContent_MakeAvailable(llm-cpp)

target_link_libraries(app PRIVATE llm::cache llm::json)
```

`add_subdirectory(llm-cpp)` works the same way. Tests and install rules are off
when llm-cpp is not the top-level project.

## Installed package

```sh
cmake -S llm-cpp -B build -DCMAKE_INSTALL_PREFIX=/opt/llm-cpp -DLLM_CPP_BUILD_TESTS=OFF
cmake --install build
```

```cmake
find_package(llm-cpp 1.1 CONFIG REQUIRED COMPONENTS cache json stream)
target_link_libraries(app PRIVATE llm::cache llm::json llm::stream)
```

Point CMake at the prefix with `-DCMAKE_PREFIX_PATH=/opt/llm-cpp`. A build tree
works too: `-Dllm-cpp_DIR=<llm-cpp build dir>`. `llm-cpp.pc` is installed for
pkg-config (`pkg-config --cflags llm-cpp`; add `-lcurl` yourself for the network
libraries).

## Implementation macro

Every header is declarations by default. In exactly one `.cpp` file, define the
library's macro before including it:

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include <llm/llm_cache.hpp>
```

Both `#include <llm/llm_cache.hpp>` and `#include "llm_cache.hpp"` resolve, so
code written against the single-file repos compiles unchanged.

## Targets and dependencies

| Target | Header | Needs |
|---|---|---|
| `llm::llm` | `<llm/llm.hpp>` + all of the below | everything below |
| `llm::ab` | `llm_ab.hpp` | libcurl |
| `llm::agent` | `llm_agent.hpp` | libcurl |
| `llm::audio` | `llm_audio.hpp` | libcurl |
| `llm::batch` | `llm_batch.hpp` | libcurl, Threads |
| `llm::cache` | `llm_cache.hpp` | Threads |
| `llm::chat` | `llm_chat.hpp` | libcurl |
| `llm::compress` | `llm_compress.hpp` | none |
| `llm::compress_summarize` | `llm_compress.hpp` with `LLM_COMPRESS_SUMMARIZE` | libcurl |
| `llm::cost` | `llm_cost.hpp` | none |
| `llm::embed` | `llm_embed.hpp` | libcurl |
| `llm::eval` | `llm_eval.hpp` | libcurl (the header itself includes `<curl/curl.h>`) |
| `llm::finetune` | `llm_finetune.hpp` | libcurl, Threads |
| `llm::format` | `llm_format.hpp` | none |
| `llm::guard` | `llm_guard.hpp` | none |
| `llm::json` | `llm_json.hpp` | none |
| `llm::log` | `llm_log.hpp` | none |
| `llm::mock` | `llm_mock.hpp` | Threads |
| `llm::parse` | `llm_parse.hpp` | none |
| `llm::pool` | `llm_pool.hpp` | Threads |
| `llm::rag` | `llm_rag.hpp` | libcurl |
| `llm::rank` | `llm_rank.hpp` | libcurl |
| `llm::retry` | `llm_retry.hpp` | Threads |
| `llm::router` | `llm_router.hpp` | none |
| `llm::stream` | `llm_stream.hpp` | libcurl (the header itself includes `<curl/curl.h>`), Threads |
| `llm::template` | `llm_template.hpp` | none |
| `llm::trace` | `llm_trace.hpp` | none |
| `llm::vision` | `llm_vision.hpp` | libcurl |

libcurl is optional for the package as a whole. The network targets link
`CURL::libcurl` when it has been found (by llm-cpp's config, or by your project
before it links them). Naming one of them in `COMPONENTS` makes libcurl required,
so a missing libcurl fails at configure time instead of link time. The headers do
not depend on each other, and none of them uses nlohmann/json, cpp-httplib or any
other library: JSON handling is built in (and `llm::json` is available on its own).

## Building the tests

```sh
cmake -S . -B build -DLLM_CPP_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Without libcurl the network libraries' tests and examples are skipped.
`-DLLM_CPP_BUILD_EXAMPLES=OFF` skips the 106 upstream examples.

## Updating the headers

`include/llm/SOURCES.md` records the repository, commit and SHA-256 of every
header. `python tools/sync_headers.py` refreshes them, the upstream examples in
`tests/examples/` and the upstream tests in `tests/upstream/` from each
library's `main` branch.
