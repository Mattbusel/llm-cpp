# llm-cpp

English | [简体中文](README.zh-CN.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

[![Latest release](https://img.shields.io/github/v/release/Mattbusel/llm-cpp)](https://gitlab.com/mattbusel/llm-cpp/-/releases) [![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

Add ChatGPT or Claude to C++ with one file.

26 single-header C++17 libraries. No SDK.

- Streams, retries and caches replies
- Prices prompts before you send them
- Forces JSON that matches a schema
- Strips personal data and API keys

## Install

```bash
curl -fsSLO https://gitlab.com/mattbusel/llm-cache/-/raw/main/include/llm_cache.hpp
```

## Use

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"
```

Do that in one `.cpp`. Compile as C++17.

[All 26 headers (.zip)](https://gitlab.com/mattbusel/llm-cpp/-/releases/permalink/latest/downloads/llm-cpp-headers.zip) · [Catalogue site](https://llm-cpp.vercel.app/) · [Reference](docs/REFERENCE.md)

More: [docs/DETAILS.md](docs/DETAILS.md)

License: MIT
