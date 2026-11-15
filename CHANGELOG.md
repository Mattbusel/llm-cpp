# Changelog

All notable changes to llm-cpp. Versions follow [Semantic Versioning](https://semver.org/).

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

[1.0.0]: https://github.com/Mattbusel/llm-cpp/compare/v0.2.0...v1.0.0
[0.2.0]: https://github.com/Mattbusel/llm-cpp/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Mattbusel/llm-cpp/releases/tag/v0.1.0
