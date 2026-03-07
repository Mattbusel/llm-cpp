# Contributing to llm-cpp

llm-cpp is a collection of 26 single-header C++20 libraries. Each header is intentionally self-contained — zero dependencies, drop-in integration.

## What we want

- **New headers** — if you need a primitive that fits the single-header philosophy, propose it
- **Bug fixes** — correctness issues, edge cases, compiler compatibility
- **Performance improvements** — anything that reduces overhead on the hot path
- **New providers** — extending existing headers to support additional LLM APIs
- **Tests** — the `tests/` directory uses a lightweight harness; more coverage is always welcome

## What we don't want

- Headers that require external dependencies (defeats the purpose)
- C++17 or earlier features (this is a C++20 library)
- Anything that adds build system requirements beyond a C++20 compiler

## How to contribute

1. Fork and clone
2. Add or modify a header in the appropriate location
3. Add tests to `tests/`
4. Verify with `g++ -std=c++20 -Wall -Wextra tests/your_test.cpp -o test && ./test` (or MSVC equivalent)
5. Open a PR

## Questions

Open a [Discussion](https://github.com/Mattbusel/llm-cpp/discussions).
