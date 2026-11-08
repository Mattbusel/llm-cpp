# llm-cpp: details

The full README from before the bare-bones rewrite, kept word for word. Short version: [README](../README.md).

---

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="../assets/banner-dark.png">
  <img alt="llm-cpp: the llm_cache.hpp header next to a terminal that downloads it, compiles an example with MSVC and prints real cache hits and evictions" src="../assets/banner-light.png">
</picture>

# llm-cpp: single-header C++ libraries for LLM features

<!-- outcome-block -->
**Add LLM calls to a C++ app with one header. 26 single-header C++17 libraries. No Python, no SDK, no package manager.**

The official SDKs are Python and JavaScript. Each library here is one `.hpp`. Define one macro and ship.

If it doesn't do what this README says, [open an issue](https://github.com/Mattbusel/llm-cpp/issues) and I'll fix it.
<!-- outcome-block -->

English | [简体中文](../README.zh-CN.md) | [日本語](../README.ja.md) | [한국어](../README.ko.md)

**Add ChatGPT or Claude features to a C++ program by copying one file.** Streaming, retries, caching, cost estimates, RAG, structured JSON output, tool-calling agents and 19 more. Works with the OpenAI and Anthropic APIs.

For C++ games, desktop apps, trading systems, embedded tools and services that want LLM calls without a Python sidecar.

[![Latest release](https://img.shields.io/github/v/release/Mattbusel/llm-cpp)](https://gitlab.com/mattbusel/llm-cpp/-/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](../LICENSE)

## Before you wire it in

- [LLM VRAM calculator](https://tensorust.com/tools/llm-vram-calculator/?utm_source=github&utm_medium=readme&utm_campaign=llm-cpp): check if a local model fits your GPU before using these headers with llama.cpp.
- [LLM API cost calculator](https://tensorust.com/tools/llm-api-cost/?utm_source=github&utm_medium=readme&utm_campaign=llm-cpp): estimate what hosted providers will cost, next to `llm_cost.hpp`.
- [Run an LLM in the browser: WebGPU 63 tok/s vs WASM 1.6](https://tensorust.com/articles/run-llm-in-browser-webgpu/?utm_source=github&utm_medium=readme&utm_campaign=llm-cpp): if you can skip the server.

## Download

### [Download all 26 headers (.zip)](https://gitlab.com/mattbusel/llm-cpp/-/releases/permalink/latest/downloads/llm-cpp-headers.zip)

The zip has `include/` with every `llm_*.hpp`, six offline examples and a short README. Or grab just one:

```bash
curl -fsSLO https://gitlab.com/mattbusel/llm-cache/-/raw/main/include/llm_cache.hpp
```

**[Browse the catalogue site](https://llm-cpp.vercel.app/)**: filter the 26 libraries, see real output, get an install command for the headers you pick.

## Which header do I need?

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="img/picker-dark.svg">
  <img alt="A chart of sixteen tasks mapped to headers. Stream a reply: llm_stream. Retry and fail over: llm_retry. Skip repeat prompts: llm_cache. Price a prompt: llm_cost. Valid JSON: llm_format plus llm_json. Chatbot with memory: llm_chat plus llm_retry. Documents Q and A: llm_rag plus llm_rank. Long chats: llm_compress. Tool calling: llm_agent. Scrub PII and keys: llm_guard. Cheaper model routing: llm_router. Logging: llm_log plus llm_trace. Batch prompts: llm_batch. Tests without network: llm_mock. Images: llm_vision. Audio: llm_audio. Green headers are offline; orange ones need libcurl." src="img/picker-light.svg" width="830">
</picture>

Green headers use only the C++ standard library. Orange ones call the OpenAI or Anthropic API and need libcurl. All 26, with one line each: [docs/REFERENCE.md](REFERENCE.md).

## How it works

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="img/how-it-works-dark.svg">
  <img alt="Four steps. 1: curl llm_cache.hpp into your project. 2: the header's lines 1 to 85 are declarations, lines 86 to 210 are the implementation behind #ifdef LLM_CACHE_IMPLEMENTATION. 3: cache.cpp defines LLM_CACHE_IMPLEMENTATION before including it and gets the code; any other file just includes it. 4: cl compiles cache.cpp and the program prints one cache hit, four misses and two evictions." src="img/how-it-works-light.svg" width="830">
</picture>

Every header uses this stb-style pattern. Learn one, you know all 26. Using several? Give each implementation its own `.cpp`: [docs/USING-SEVERAL.md](USING-SEVERAL.md).

## Examples (real output, no API key)

Real programs in [`examples/offline`](../examples/offline). On every push, CI rebuilds them with g++ against each current header. It fails if the output below changes.

**Strip personal data and API keys before a prompt leaves your app** ([guard.cpp](../examples/offline/guard.cpp), llm-guard):

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="../assets/example-guard-dark.png">
  <img alt="guard.cpp scans a prompt for an email, a card number and an API key, scores it 0.75 for injection and prints the scrubbed text" src="../assets/example-guard-light.png" width="830">
</picture>

**Price one prompt across models. Block a call that would cost more than a cent** ([cost.cpp](../examples/offline/cost.cpp), llm-cost):

```text
C:\demo> cl /nologo /std:c++17 /EHsc cost.cpp && cost.exe
cost.cpp
gpt-6-luna          4080 tokens  0.0408¢
gpt-4o-mini         4080 tokens  0.0612¢
claude-haiku-4-5    4080 tokens  0.4080¢
gpt-6-sol           4080 tokens  0.8160¢
claude-sonnet-5     4080 tokens  0.8160¢
gpt-4o              4080 tokens  $0.0102
claude-sonnet-4-5   4080 tokens  $0.0122
claude-opus-5-5     4080 tokens  $0.0163
claude-opus-4-5     4080 tokens  $0.0204
gpt-6-astra         4080 tokens  $0.0408
gpt-4-turbo         4080 tokens  $0.0408
claude-fable-5-1    4080 tokens  $0.0408

blocked: Budget exceeded: estimated $0.0204 > limit $0.0100 (4080 tokens on claude-opus-4-5)
```

**Get JSON that matches a schema. Re-asks until the model gets it right** ([format.cpp](../examples/offline/format.cpp), llm-format; a lambda stands in for the model):

```text
valid: yes after 2 attempt(s)
{
  "priority": 1,
  "tags": [
    "auth"
  ],
  "title": "Login fails"
}
error: Field "title" has wrong type: expected string
error: Missing required field: "priority"
error: Missing required field: "tags"
```

Also in the folder: [cache.cpp](../examples/offline/cache.cpp) (the output in the diagram above), [json.cpp](../examples/offline/json.cpp) and [compress.cpp](../examples/offline/compress.cpp). Compiled with MSVC 19.44 on 2026-09-28. Token counts in llm-cost are estimates.

## Use it in 3 steps

1. **Copy** the header into your project (from the zip, or `curl -fsSLO` as above).
2. **Turn on the code** in exactly one `.cpp`:
   ```cpp
   #define LLM_CACHE_IMPLEMENTATION
   #include "llm_cache.hpp"
   ```
   Every other file just writes `#include "llm_cache.hpp"`.
3. **Compile as C++17**: `g++ -std=c++17 main.cpp` or `cl /std:c++17 /EHsc main.cpp`. Headers marked libcurl also need `-lcurl` (preinstalled on macOS, `apt install libcurl4-openssl-dev`, `vcpkg install curl`).

## Documentation

| | |
|---|---|
| [Catalogue site](https://llm-cpp.vercel.app/) | Filter all 26, real output, generated install commands |
| [docs/REFERENCE.md](REFERENCE.md) | Every library with what it does and what it needs, requirements, status |
| [docs/USING-SEVERAL.md](USING-SEVERAL.md) | Combining headers: a stream + retry + log program, an offline RAG pipeline |
| [examples/offline](../examples/offline) | Six programs that need no API key, with their committed output |
| [Releases](https://gitlab.com/mattbusel/llm-cpp/-/releases) | The headers zip, built and link-checked by CI |

Each library has its own repo (`gitlab.com/mattbusel/llm-<name>`). Issues and PRs welcome there.

## Hire the author

**Need this on your product?** I take a few client builds: LLM features, iOS apps, performance work. Fixed price. [Services and pricing](https://tensorust.com/) · [Email](mailto:mattbusel@gmail.com) · [LinkedIn](https://www.linkedin.com/in/matthewbusel/)
