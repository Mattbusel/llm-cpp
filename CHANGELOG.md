# Changelog

All notable changes to llm-cpp. Versions follow [Semantic Versioning](https://semver.org/).

## [1.4.0] - 2026-11-18

`llm_stream.hpp` now has a standalone Server-Sent Events parser, so a project
with its own HTTP client can use llm-cpp for the SSE side of streaming
(llm-stream [`ea23a2d`](https://github.com/Mattbusel/llm-stream/commit/ea23a2d)).

### Added

- `llm::SseParser` (incremental: `feed(chunk, on_event)`, `finish(on_event)`) and
  `llm::parse_sse(body)`. They follow the WHATWG event-stream rules: LF, CRLF or
  CR line ends, also split across chunks, a leading BOM, `:` comments, one
  optional space after the colon, multi-line `data:`, `id:` and `retry:`, and
  events with no data dropped. `finish()` also delivers a last event that has no
  closing blank line.
- The parser is header-only (inline) and needs no `LLM_STREAM_IMPLEMENTATION`.
  Define `LLM_STREAM_NO_CURL` before including `llm_stream.hpp` to get only the
  parser, without `<curl/curl.h>` and the network API (for example in a
  WebAssembly build or a codebase that already has an HTTP stack).
- Tests: `upstream_stream_test_sse` (parser, every chunk size, built with
  `LLM_STREAM_NO_CURL`); `upstream_stream_test_responses_sse` now also covers the
  Anthropic path.

### Changed

- The libcurl streaming functions run on the same parser. `event:` and `data:`
  without a space after the colon, lone CR line ends and multi-line `data:` are
  now handled, and a final event without a closing blank line is no longer lost.

## [1.3.0] - 2026-11-17

Builds cleanly inside strict codebases: every header compiles warning-free at
`-Wall -Wextra -Wpedantic -Wshadow` (GCC, Clang) and `/W4 /permissive-` (MSVC),
and CI now enforces it with `-Werror` / `/WX`. No API changes.

### Fixed

- 15 headers did an unguarded `#define NOMINMAX`. A project that passes
  `-DNOMINMAX` (common on Windows) got a macro redefinition, MSVC C4005 or
  GCC/Clang "NOMINMAX redefined", which fails `/WX` and `-Werror` builds. They now
  define it only on Windows and only when it is not defined yet: ab, audio,
  batch, chat, compress, embed, eval, finetune, guard, json, mock, rag, rank,
  router, vision.
- 8 headers failed to compile after `#include <windows.h>` without `NOMINMAX`
  (error C2589 on `std::min`/`std::max`): audio, batch, eval, guard, parse, rag,
  rank, router. The calls are `(std::min)(...)` / `(std::max)(...)` now, as
  `llm_stream.hpp` has been since 1.2.1.
- `LLM_TRACE` pasted `__LINE__` unexpanded, so two `LLM_TRACE(...)` calls in one
  scope declared the same variable and did not compile. It expands the line
  number first now (llm-trace [`daf582a`](https://github.com/Mattbusel/llm-trace/commit/daf582af233973be47d364fda282faeddc640ac5)).
- Warnings in consuming builds: unused `jesc()` in `llm_compress.hpp` without
  `LLM_COMPRESS_SUMMARIZE`, unused `is_hex()` and a local in `llm_guard.hpp`, a
  set-but-unused flag in `llm_parse.hpp`, and an implicit `size_t` to `double`
  conversion in `llm_rank.hpp` (MSVC C4244).
- `LLM_CPP_VERSION_MINOR` / `_PATCH` said 1.1.0 in the 1.2.0 and 1.2.1 releases
  while `LLM_CPP_VERSION` said 1.2.x. They agree again, and `link_all` fails if
  they ever drift.

### Added

- `LLM_CPP_STRICT_WARNINGS` CMake option (off by default): the header tests,
  `link_all` and the new tests build with `-Wall -Wextra -Wpedantic -Wshadow
  -Werror`, or `/W4 /WX /permissive-` on MSVC. CI turns it on everywhere and adds
  a Clang job on Ubuntu.
- Tests: `hygiene` (every header with `NOMINMAX` predefined, two `LLM_TRACE` in
  one scope), `windows_h_<name>` (Windows: each header after `<windows.h>`), and
  `header_compress_summarize` (the `LLM_COMPRESS_SUMMARIZE` path).
- `llm-cpp.pc` declares `Requires: libcurl` when libcurl was found at configure
  time, so `pkg-config --cflags --libs llm-cpp` carries the network headers'
  dependency.
- [docs/INTEGRATION.md](docs/INTEGRATION.md): the five-line integration and what
  a reviewer asks before adding the dependency (compilers, warnings, run-time
  behaviour, versioning). [examples/fetchcontent/](examples/fetchcontent/) is that
  integration as a project; CI builds and runs it on every platform.
- [SECURITY.md](SECURITY.md), with private vulnerability reporting turned on.

Headers re-synced from their repositories (commits in
[include/llm/SOURCES.md](include/llm/SOURCES.md)).

## [1.2.1] - 2026-11-16

### Fixed

- MSVC: `llm_stream.hpp` 1.2.0 did not compile with MSVC when libcurl's headers bring in
  `windows.h`, whose `min` macro turned `std::min(...)` into error C2589. It is
  `(std::min)(...)` now (llm-stream [`08dcc05`](https://github.com/Mattbusel/llm-stream/commit/08dcc05487a59a43a5d4107441cf7d30c5998ae4)).

## [1.2.0] - 2026-11-16

OpenAI's Responses API in `llm_stream.hpp`.

### Added

- llm-stream (now [`08b091a`](https://github.com/Mattbusel/llm-stream/commit/08b091a344b0ece7c8e6e5b99610df3e2b5e3b32)):
  `llm::stream_openai_responses(prompt, config, on_token, on_done, on_error)` posts to
  `/v1/responses` (`input`, `instructions` from `system_prompt`, `max_output_tokens`;
  `temperature` left out for the `o1`/`o3`/`o4`/`gpt-5` models that reject it) and streams
  `response.output_text.delta` text. `StreamStats` gains `input_tokens`, `output_tokens`
  (from `response.completed`, -1 when not reported) and `response_id`.
  `response.failed`, `response.incomplete` and `error` events go to `on_error` with the
  API's message, and a stream that ends without `response.completed` is an error.
- HTTP errors from every `llm_stream.hpp` call now include the response body's
  `error.message` (they used to say only "HTTP error 400").
- `tests/upstream/stream/test_responses_sse.cpp`: feeds Responses API SSE (in 1, 7 and
  4096 byte chunks), failure, error and incomplete events to the parser, checks the request
  body, and that Chat Completions parsing is unchanged. Runs offline.
- Examples `stream/responses_stream.cpp` and `stream/anthropic_stream.cpp` (compiled in CI).

## [1.1.0] - 2026-11-15

`llm_cost.hpp` now prices 131 models from 10 providers instead of 12, and the
table is generated from [llm-prices](https://github.com/Mattbusel/llm-prices), so
it stays current with one command.

### Added

- `tools/gen_cost_table.py`: rewrites the `Provider` enum and the `llm::models`
  table in `llm_cost.hpp` from llm-prices' `prices.json` (standard per-token
  rates, checked against each provider's pricing page). `--check` exits 1 when
  the header is out of date. See [docs/COST-TABLE.md](docs/COST-TABLE.md).
- `.github/workflows/cost-table.yml`: runs the check every Monday, on demand,
  and when the header or the script changes, so a price change in llm-prices
  shows up here as a failed run.
- llm-cost (now [`d4f3240`](https://github.com/Mattbusel/llm-cost/commit/d4f3240060c76803a721311efe9bcc03ccde5ff5)):
  providers `Google`, `DeepSeek`, `Mistral`, `XAI`, `Cohere`, `Groq`, `Together`
  and `Fireworks`; `find_model(id)` and `find_model(id, provider)` (ids and
  aliases such as `claude-sonnet-4-5-20250929`, `deepseek-chat`,
  `mistral-large-latest`); `all_models()`; `compare_costs(text, models)`.
- `tests/upstream/cost/cost_table.cpp`: llm-cost's table and lookup tests, run
  by ctest.

### Changed

- `compare_costs(text)` compares every listed chat model (126 today), with a
  stable sort so ties print in the same order everywhere. `gpt-4-turbo` is
  retired upstream and left out of it; `llm::models::GPT4_TURBO` still works.
- `Model::context_window` of 0 means "not known" and is never reported as
  exceeded. The twelve original models keep their context windows, names,
  ids and prices.
- `examples/offline/cost.cpp` picks ten models by id from four more providers.
- `tools/build_site.py` reproduces the published site (GitLab links and the
  language switcher), so the "Site is up to date" CI job passes again.

## [1.0.0] - 2026-11-15

llm-cpp is now an installable package, not only an index of the 26 libraries.

### Added

- `include/llm/`: all 26 single-header libraries vendored byte for byte from their
  repositories, with the source repo, commit and SHA-256 of each in
  `include/llm/SOURCES.md`. `tools/sync_headers.py` refreshes them.
- `include/llm/llm.hpp`: umbrella header with `LLM_CPP_VERSION` macros.
- CMake package (CMake 3.14+): one header-only INTERFACE target per library
  (`llm::ab`, `llm::agent`, `llm::audio`, `llm::batch`, `llm::cache`, `llm::chat`,
  `llm::compress`, `llm::cost`, `llm::embed`, `llm::eval`, `llm::finetune`,
  `llm::format`, `llm::guard`, `llm::json`, `llm::log`, `llm::mock`, `llm::parse`,
  `llm::pool`, `llm::rag`, `llm::rank`, `llm::retry`, `llm::router`, `llm::stream`,
  `llm::template`, `llm::trace`, `llm::vision`), `llm::compress_summarize`, and the
  umbrella `llm::llm`.
- `install()` rules, exported targets, `llm-cppConfig.cmake` and
  `llm-cppConfigVersion.cmake` (SameMajorVersion), so
  `find_package(llm-cpp CONFIG REQUIRED [COMPONENTS ...])` works after install, from
  the build tree, or through `add_subdirectory` / `FetchContent`. A `llm-cpp.pc`
  pkg-config file is installed too.
- Optional dependencies per component: the 12 network libraries (ab, agent, audio,
  batch, chat, embed, eval, finetune, rag, rank, stream, vision) and
  `compress_summarize` link `CURL::libcurl` when it is found; asking for one of them
  in `COMPONENTS` makes libcurl required. batch, cache, finetune, mock, pool, retry
  and stream link `Threads::Threads`. No other external library is used.
- `tests/`: a compile-and-link test per header (implementation in one translation
  unit, the header again in a second), all implementations linked into one binary,
  the offline examples diffed against their committed output, llm-retry's own
  regression tests, and every upstream example compiled. Option
  `LLM_CPP_BUILD_TESTS`.
- `.github/workflows/cmake.yml`: builds and runs the tests on Ubuntu (GCC), macOS
  (Apple Clang) and Windows (MSVC, libcurl from vcpkg), then installs the package and
  builds a separate project against it with `find_package`.

### Changed

- The release workflow and CI build from the vendored headers in `include/llm/`
  instead of downloading each library's `main` branch, so a tag always ships the
  exact headers it was tested with.

## [0.2.0] - 2026-09-30

- All 26 headers in one download (`llm-cpp-headers.zip`), header picker and
  how-it-works diagrams in the README.

## [0.1.0] - 2026-03-07

- Index of the 26 libraries, grouped by category, with a decision tree.

[1.1.0]: https://github.com/Mattbusel/llm-cpp/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/Mattbusel/llm-cpp/compare/v0.2.0...v1.0.0
[0.2.0]: https://github.com/Mattbusel/llm-cpp/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Mattbusel/llm-cpp/releases/tag/v0.1.0
