# Adding llm-cpp to an existing codebase

What a reviewer usually asks before taking a new dependency, answered in one
place. For every packaging option see [PACKAGE.md](PACKAGE.md); for the API of
each library see [REFERENCE.md](REFERENCE.md).

## The integration

```cmake
include(FetchContent)
FetchContent_Declare(llm-cpp
  URL https://github.com/Mattbusel/llm-cpp/archive/refs/tags/v1.3.0.tar.gz)
FetchContent_MakeAvailable(llm-cpp)
target_link_libraries(app PRIVATE llm::cost llm::json)   # only what you use
```

```cpp
#define LLM_COST_IMPLEMENTATION      // in exactly one .cpp
#include <llm/llm_cost.hpp>
auto tc = llm::count(prompt, llm::models::GPT4O_MINI);
llm::assert_budget(tc, 0.01);        // throws if the prompt would cost more
```

[examples/fetchcontent/](../examples/fetchcontent/) is this as a project, and CI
builds and runs it on Linux, macOS and Windows.

Already vendor headers? Copy only the files you need from `include/llm/`. They
do not include each other, so one header is a complete dependency.

## What you take on

| Question | Answer |
|---|---|
| Language | C++17. Errors are reported with exceptions (for example `assert_budget` throws `std::runtime_error`). |
| Build | CMake 3.14+ (`llm::<name>` targets, `find_package(llm-cpp CONFIG)`), or pkg-config, or plain `-I include/llm`. |
| Third-party code | None in the offline libraries (standard library only). The 12 network libraries need libcurl and nothing else. No JSON library, no HTTP library, no Boost. |
| Linking | Header-only. The implementation compiles into the one `.cpp` that defines `LLM_<NAME>_IMPLEMENTATION`. |
| Size | 26 headers, about 9,500 lines in total. Most are 200 to 500 lines. |
| License | MIT, for every header ([LICENSE](../LICENSE), and per header in [include/llm/SOURCES.md](../include/llm/SOURCES.md)). |
| Provenance | Each header is a byte-for-byte copy from its own repository; `SOURCES.md` records the commit and SHA-256. |

## Compilers and warnings

CI ([cmake.yml](../.github/workflows/cmake.yml)) builds every header, every
implementation linked into one binary, and the libraries' own examples on:

- GCC on `ubuntu-latest` (with and without libcurl) and Clang on `ubuntu-latest`
- Apple Clang on `macos-latest`
- MSVC on `windows-latest` (libcurl from vcpkg)

The header tests are built with `LLM_CPP_STRICT_WARNINGS=ON`: `-Wall -Wextra
-Wpedantic -Wshadow -Werror` on GCC and Clang, `/W4 /WX /permissive-` on MSVC.
A header that would add a warning to a strict consuming build fails CI here
first.

Windows specifics that are tested:

- Every header compiles after `#include <windows.h>` without `NOMINMAX`
  (`std::min`/`std::max` are written `(std::min)(...)`).
- Every header compiles with `NOMINMAX` already defined by the project. The
  headers that pull in libcurl define it only on Windows and only when it is not
  defined yet.

## What the code does at run time

- The only hosts in the headers are `api.openai.com` and `api.anthropic.com`,
  used by the network libraries you call. There is no telemetry.
- No header reads environment variables. API keys come from the config structs
  you fill in.
- Files are written only through calls you make, to paths you pass or set in a
  config. Two configs have a default: `llm::LogConfig::filepath` is
  `llm_calls.jsonl` (set it to `""` to log nowhere) and `llm::RagConfig::index_path`
  is `.rag_index` (written by `save_index()`).
- TLS uses libcurl's defaults, which verify the server certificate. No header
  turns verification off.

## Versioning

llm-cpp follows [Semantic Versioning](https://semver.org/). The public API is
what the headers declare outside `detail` and `detail_*` namespaces, the CMake target names,
the `LLM_<NAME>_IMPLEMENTATION` macros and the `LLM_CPP_VERSION*` macros.
Breaking changes to those wait for a new major version and are listed in
[CHANGELOG.md](../CHANGELOG.md). `find_package(llm-cpp 1.3 CONFIG)` accepts any
later 1.x.

## Maintenance

Matthew Busel maintains llm-cpp and the 26 libraries, keeps the model price
table current from [llm-prices](https://github.com/Mattbusel/llm-prices) (a
weekly CI job fails when it drifts), and takes issues on GitHub. Security
reports: see [SECURITY.md](../SECURITY.md).
