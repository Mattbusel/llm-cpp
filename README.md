<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/banner-dark.png">
  <img alt="llm-cpp: the llm_cache.hpp header next to a terminal that downloads it, compiles an example with MSVC and prints real cache hits and evictions" src="assets/banner-light.png">
</picture>

[![CI](https://github.com/Mattbusel/llm-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/Mattbusel/llm-cpp/actions/workflows/ci.yml)

**[Browse the catalogue](https://mattbusel.github.io/llm-cpp/)**: filter all 26 libraries by what they need, see real output, and get an install command for the headers you pick.

**26 single-header C++17 libraries for building LLM features into native code.** Streaming, retries, caching, cost estimation, RAG, reranking, tracing, structured output, agents and more. Each library is one `.hpp` file you copy into your project.

Most LLM tooling assumes Python or Node. If you are shipping a game, a desktop app, a trading system, an embedded tool or a C++ service, you usually end up hand-rolling HTTP calls, retry loops and JSON parsing. llm-cpp is that plumbing, split into small pieces so you take only what you need: no SDK, no package manager, no framework. The offline libraries have no dependencies at all; the ones that talk to OpenAI or Anthropic need only libcurl.

## Start here

| I want to... | Use |
|---|---|
| Call a model and stream tokens | [llm-stream](https://github.com/Mattbusel/llm-stream) |
| Build a chatbot with memory | [llm-chat](https://github.com/Mattbusel/llm-chat) + [llm-retry](https://github.com/Mattbusel/llm-retry) |
| Answer questions over my documents | [llm-parse](https://github.com/Mattbusel/llm-parse) + [llm-embed](https://github.com/Mattbusel/llm-embed) or [llm-rag](https://github.com/Mattbusel/llm-rag) + [llm-rank](https://github.com/Mattbusel/llm-rank) |
| Get valid JSON back every time | [llm-format](https://github.com/Mattbusel/llm-format) + [llm-json](https://github.com/Mattbusel/llm-json) |
| Let the model call my C++ functions | [llm-agent](https://github.com/Mattbusel/llm-agent) |
| Know what my calls cost and where time goes | [llm-cost](https://github.com/Mattbusel/llm-cost) + [llm-log](https://github.com/Mattbusel/llm-log) + [llm-trace](https://github.com/Mattbusel/llm-trace) |
| Unit-test LLM code without the network | [llm-mock](https://github.com/Mattbusel/llm-mock) |

## The libraries

"libcurl" means the implementation makes HTTPS calls (OpenAI and/or Anthropic APIs). "none" means it is fully offline and uses only the standard library.

### Core

| Library | What it does | Needs |
|---|---|---|
| [llm-stream](https://github.com/Mattbusel/llm-stream) | Stream OpenAI and Anthropic chat responses token by token over SSE | libcurl |
| [llm-retry](https://github.com/Mattbusel/llm-retry) | Exponential backoff with jitter, provider failover and a circuit breaker | none |
| [llm-cost](https://github.com/Mattbusel/llm-cost) | Approximate token counts and cost estimates for built-in OpenAI and Anthropic models, budget checks | none |
| [llm-cache](https://github.com/Mattbusel/llm-cache) | LRU response cache with TTL and hit/miss stats, so identical prompts skip the API | none |
| [llm-format](https://github.com/Mattbusel/llm-format) | Define a schema, validate model JSON against it, and re-prompt until the output conforms | none |
| [llm-json](https://github.com/Mattbusel/llm-json) | Small JSON parser and builder for request bodies and model output | none |

### Data and retrieval

| Library | What it does | Needs |
|---|---|---|
| [llm-parse](https://github.com/Mattbusel/llm-parse) | Strip HTML and markdown, extract titles, links, headings and code blocks, chunk text | none |
| [llm-embed](https://github.com/Mattbusel/llm-embed) | OpenAI embeddings, cosine/dot/euclidean similarity and a small on-disk vector store | libcurl |
| [llm-rag](https://github.com/Mattbusel/llm-rag) | End-to-end RAG: chunk, embed, persist an index, retrieve top-k and answer | libcurl |
| [llm-rank](https://github.com/Mattbusel/llm-rank) | Rerank passages with offline BM25, LLM relevance scoring, or a hybrid of both | libcurl (linked; BM25 itself is offline) |
| [llm-compress](https://github.com/Mattbusel/llm-compress) | Shrink conversation history: head/tail/smart truncation, sliding window, LLM summary | none (libcurl only with `LLM_COMPRESS_SUMMARIZE`) |
| [llm-batch](https://github.com/Mattbusel/llm-batch) | Run a JSONL file of prompts through a thread pool with rate limiting and resumable checkpoints | libcurl |

### Operations and testing

| Library | What it does | Needs |
|---|---|---|
| [llm-log](https://github.com/Mattbusel/llm-log) | Structured JSONL log of every call with latency, tokens and cost, plus query and summary | none |
| [llm-trace](https://github.com/Mattbusel/llm-trace) | RAII spans with parent/child nesting, token and cost attributes, OTLP-style JSON export | none |
| [llm-pool](https://github.com/Mattbusel/llm-pool) | Worker pool with priority queue and requests-per-minute and tokens-per-minute limits | none |
| [llm-mock](https://github.com/Mattbusel/llm-mock) | Fake LLM with scripted, pattern, random or echo responses, simulated latency and streaming | none |
| [llm-eval](https://github.com/Mattbusel/llm-eval) | Run a prompt N times, measure consistency, compare models or prompts, score responses | libcurl |
| [llm-ab](https://github.com/Mattbusel/llm-ab) | A/B test prompts or models with Welch's t-test, Cohen's d and custom scorers | libcurl |

### Application features

| Library | What it does | Needs |
|---|---|---|
| [llm-chat](https://github.com/Mattbusel/llm-chat) | Multi-turn conversation with token-budget trimming, pinned system prompt, save and restore | libcurl |
| [llm-agent](https://github.com/Mattbusel/llm-agent) | Tool-calling agent loop: register C++ lambdas as tools and let the model call them | libcurl |
| [llm-vision](https://github.com/Mattbusel/llm-vision) | Send images (file or URL) plus a prompt to OpenAI or Anthropic vision models | libcurl |
| [llm-template](https://github.com/Mattbusel/llm-template) | Mustache-style prompt templates with loops, conditionals and token-budget truncation | none |
| [llm-router](https://github.com/Mattbusel/llm-router) | Pick a model per prompt from a complexity score and a cost, latency, quality or budget strategy | none |
| [llm-guard](https://github.com/Mattbusel/llm-guard) | Detect and scrub PII (email, phone, SSN, card numbers, API keys) and score prompt-injection risk | none |
| [llm-audio](https://github.com/Mattbusel/llm-audio) | Whisper transcription and translation, and text-to-speech, via the OpenAI API | libcurl |
| [llm-finetune](https://github.com/Mattbusel/llm-finetune) | OpenAI fine-tuning lifecycle: write JSONL, upload, create, poll, cancel, list models | libcurl |

## Why single-header

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/one-file-dark.png">
  <img alt="The llm-cache repo on the left with only include/llm_cache.hpp highlighted, copied with curl into third_party/ of your project on the right" src="assets/one-file-light.png">
</picture>

- **Nothing to install.** `curl -O` one file, `#include` it. It works the same with CMake, Make, Bazel, MSBuild or a one-line `g++` command.
- **You can read all of it.** Each library is 210 to 572 lines; all 26 together are 8,923. When something misbehaves you open one file, not a dependency tree.
- **You pay for what you use.** Need retries and a cache? Take two headers. Nothing else is pulled in, and the offline ones add no link dependencies at all.
- **Easy to vendor.** Copy the headers into `third_party/`, pin them in your own repo, patch them if you need to. No version resolver involved.

## Install

Grab the headers you want (each lives at `include/<name>.hpp` in its repo):

```bash
mkdir -p third_party && cd third_party
for lib in stream retry log; do
  curl -fsSLO https://raw.githubusercontent.com/Mattbusel/llm-$lib/main/include/llm_$lib.hpp
done
```

Every header follows the stb-style pattern: include it anywhere for the declarations, and in exactly one `.cpp` file define `LLM_<NAME>_IMPLEMENTATION` before including it to compile the implementation.

## Real output, no API key

Six of the offline libraries have complete example programs in [`examples/offline`](examples/offline), with the output they printed committed next to them. CI downloads each library's current header, builds every example with g++ and diffs the output, so these stay honest.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/example-guard-dark.png">
  <img alt="guard.cpp scans a prompt for an email, a card number and an API key, scores it 0.75 for injection and prints the scrubbed text" src="assets/example-guard-light.png">
</picture>

| Example | Shows |
|---|---|
| [cache.cpp](examples/offline/cache.cpp) | LRU cache: case-insensitive hits, evictions, stats |
| [cost.cpp](examples/offline/cost.cpp) | Price one prompt across the built-in models, block a call over budget |
| [guard.cpp](examples/offline/guard.cpp) | Find and scrub PII and API keys, score prompt injection |
| [format.cpp](examples/offline/format.cpp) | Validate JSON against a schema and re-prompt until it conforms |
| [json.cpp](examples/offline/json.cpp) | Build a request body, read a response, reject bad input |
| [compress.cpp](examples/offline/compress.cpp) | Keep a long chat inside a token budget with a sliding window |

```bash
curl -fsSLO https://raw.githubusercontent.com/Mattbusel/llm-guard/main/include/llm_guard.hpp
g++ -std=c++17 -I. examples/offline/guard.cpp -o guard && ./guard
```

## Using several together

**Give each implementation its own `.cpp` file.** Several headers use the same internal helper names (for example `llm::detail::json_escape`), so defining two `*_IMPLEMENTATION` macros in one translation unit can fail to compile (llm-log with llm-stream is one such pair). In separate translation units they link together fine. As a check, all 26 implementations, each in its own `.cpp`, were compiled and linked into a single binary with MSVC 19.44 and libcurl on 2026-09-25.

```cpp
// llm_impl_log.cpp
#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"

// llm_impl_retry.cpp
#define LLM_RETRY_IMPLEMENTATION
#include "llm_retry.hpp"

// llm_impl_stream.cpp
#define LLM_STREAM_IMPLEMENTATION
#include "llm_stream.hpp"
```

Then use them together anywhere. This streams a completion, retries it on failure and writes a JSONL log line:

```cpp
// main.cpp
#include "llm_log.hpp"
#include "llm_retry.hpp"
#include "llm_stream.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "set OPENAI_API_KEY\n"; return 1; }

    llm::Config cfg;
    cfg.api_key = key;
    cfg.model   = "gpt-4o-mini";
    const std::string prompt = "Explain backpressure in one paragraph.";

    llm::Logger logger(llm::LogConfig{"calls.jsonl"});
    llm::Logger::ScopedCall call(logger, cfg.model, prompt);   // written on scope exit

    auto result = llm::with_retry<std::string>([&]() -> std::string {
        std::string text, error;
        llm::stream(prompt, cfg,
            [&](std::string_view tok) { std::cout << tok << std::flush; text += tok; },
            nullptr,
            [&](std::string_view err) { error = err; });
        if (!error.empty()) throw llm::LLMError{0, error, true};   // retry
        return text;
    });

    call.set_response(result.value);
    std::cout << "\n(" << result.attempts_used << " attempt(s))\n";
}
```

```bash
g++ -std=c++17 -O2 -Ithird_party main.cpp llm_impl_log.cpp llm_impl_retry.cpp llm_impl_stream.cpp -lcurl -o app
```

An offline pipeline needs no key and no network: clean a document with llm-parse, rank passages with llm-rank's BM25, and render the final prompt with llm-template.

```cpp
#include "llm_parse.hpp"
#include "llm_rank.hpp"
#include "llm_template.hpp"
#include <iostream>

int main() {
    std::string doc = llm::strip_html(
        "<h1>Deploying</h1><p>Run make release to build the binary.</p>"
        "<p>Copy config.yaml next to the binary.</p><p>Our office is in Berlin.</p>");
    llm::ChunkConfig cc;
    cc.chunk_size = 60;
    cc.overlap    = 0;
    auto passages = llm::chunk(doc, cc);

    std::string question = "how do I build the binary";
    auto ranked = llm::rerank_local(question, passages);

    llm::Template prompt("Answer using only this context:\n"
                         "{{#ctx}}- {{text}}\n{{/ctx}}\nQuestion: {{q}}\n");
    llm::TemplateContext ctx;
    ctx.vars["q"] = question;
    for (size_t i = 0; i < ranked.size() && i < 2; ++i)
        ctx.lists["ctx"].push_back({{"text", ranked[i].passage}});

    std::cout << prompt.render(ctx);
}
```

## Requirements

| | |
|---|---|
| Language | C++17 or later |
| Compilers | GCC, Clang, MSVC. Each library repo builds its examples in CI with CMake. |
| Network libraries | libcurl: preinstalled on macOS, `apt install libcurl4-openssl-dev` on Debian/Ubuntu, `vcpkg install curl` on Windows |
| Providers | OpenAI-compatible chat, embeddings, audio and fine-tuning endpoints; Anthropic Messages API in llm-stream and llm-vision |

## Status

These are small, focused libraries, not a full SDK. The HTTP code targets the public OpenAI and Anthropic endpoints and uses hand-written JSON handling, and token counts in llm-cost are approximations. Issues and pull requests are welcome in the individual repositories.

## Related

- [LLMTokenStreamQuantEngine](https://github.com/Mattbusel/LLMTokenStreamQuantEngine): C++20 engine that turns streaming LLM tokens into trade signals.


## Hire the author

**Need this kind of engineering on your product?** I take on a small number of client builds: LLM features, iOS apps and performance work, fixed price. [Services and pricing](https://mattbusel.github.io/) · [Email](mailto:mattbusel@gmail.com) · [LinkedIn](https://www.linkedin.com/in/matthewbusel/)
