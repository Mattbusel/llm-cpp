# llm-cpp

A suite of 19 single-header C++17 libraries for integrating large language models into native applications. Each library is a self-contained `.hpp` file — drop in what you need, define one implementation macro, and ship. No Python, no SDKs, no package manager required.

---

## The Suite

| Library | Description | Deps |
|---------|-------------|------|
| **[llm-stream](https://github.com/Mattbusel/llm-stream)** | Stream OpenAI & Anthropic responses via SSE | libcurl |
| **[llm-cache](https://github.com/Mattbusel/llm-cache)** | LRU response cache — skip identical API calls | None |
| **[llm-cost](https://github.com/Mattbusel/llm-cost)** | Token counting + cost estimation for 6 models | None |
| **[llm-retry](https://github.com/Mattbusel/llm-retry)** | Retry with exponential backoff + circuit breaker | None |
| **[llm-format](https://github.com/Mattbusel/llm-format)** | JSON schema enforcement + structured output | None |
| **[llm-embed](https://github.com/Mattbusel/llm-embed)** | Text embeddings + cosine similarity + vector store | libcurl |
| **[llm-pool](https://github.com/Mattbusel/llm-pool)** | Concurrent request pool with priority queue + rate limiting | None |
| **[llm-log](https://github.com/Mattbusel/llm-log)** | Structured JSONL logging for every LLM call | None |
| **[llm-template](https://github.com/Mattbusel/llm-template)** | Mustache-style prompt templating | None |
| **[llm-agent](https://github.com/Mattbusel/llm-agent)** | Tool-calling agent loop (OpenAI function calling) | libcurl |
| **[llm-rag](https://github.com/Mattbusel/llm-rag)** | Retrieval-augmented generation pipeline | libcurl |
| **[llm-eval](https://github.com/Mattbusel/llm-eval)** | N-run evaluation + consistency scoring + model comparison | libcurl |
| **[llm-chat](https://github.com/Mattbusel/llm-chat)** | Multi-turn conversation manager with token-budget truncation | libcurl |
| **[llm-vision](https://github.com/Mattbusel/llm-vision)** | Multimodal image+text for OpenAI and Anthropic | libcurl |
| **[llm-mock](https://github.com/Mattbusel/llm-mock)** | Mock LLM provider for unit testing — zero network | None |
| **[llm-router](https://github.com/Mattbusel/llm-router)** | Route prompts to the right model by complexity | None |
| **[llm-guard](https://github.com/Mattbusel/llm-guard)** | PII detection + prompt injection scoring — fully offline | None |
| **[llm-compress](https://github.com/Mattbusel/llm-compress)** | Context compression: truncate, sliding window, summarize | None* |
| **[llm-batch](https://github.com/Mattbusel/llm-batch)** | Batch processing with thread pool, rate limiting, checkpointing | libcurl |

*llm-compress requires libcurl only for the optional Summarize strategy.

---

## Quickstart

Libraries compose naturally. Here is a production-ready pattern using llm-log, llm-retry, and llm-stream together:

```cpp
#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"

#define LLM_RETRY_IMPLEMENTATION
#include "llm_retry.hpp"

#define LLM_STREAM_IMPLEMENTATION
#include "llm_stream.hpp"

int main() {
    llm::Logger logger("calls.jsonl");

    llm::Config cfg;
    cfg.api_key = std::getenv("OPENAI_API_KEY");
    cfg.model   = "gpt-4o-mini";

    const std::string prompt = "Explain backpressure in one paragraph.";
    auto log_id = logger.log_request(prompt, cfg.model);

    auto result = llm::with_retry<std::string>([&]() -> std::string {
        std::string output;
        llm::stream_openai(prompt, cfg,
            [&](std::string_view tok) { std::cout << tok << std::flush; output += tok; },
            [](const llm::StreamStats& s) {
                std::cout << "\n[" << s.token_count << " tokens, " << s.tokens_per_sec << " tok/s]\n";
            }
        );
        return output;
    });

    logger.log_response(log_id, result);
}
```

Another example — guard, route, and chat together:

```cpp
#define LLM_GUARD_IMPLEMENTATION
#include "llm_guard.hpp"

#define LLM_ROUTER_IMPLEMENTATION
#include "llm_router.hpp"

#define LLM_CHAT_IMPLEMENTATION
#include "llm_chat.hpp"

int main() {
    // 1. Check input for PII / injection
    auto guard = llm::scan(user_input);
    if (!guard.safe) user_input = guard.scrubbed;

    // 2. Route to the right model
    llm::RouterConfig rcfg;
    rcfg.strategy = llm::RoutingStrategy::Balanced;
    rcfg.models   = {{"gpt-4o-mini", 0.15, 0.5, 0.7, 40}, {"gpt-4o", 5.0, 1.0, 0.9, 100}};
    auto decision = llm::Router(rcfg).route(user_input);

    // 3. Send with conversation memory
    llm::ChatConfig ccfg;
    ccfg.api_key = std::getenv("OPENAI_API_KEY");
    ccfg.model   = decision.model_name;
    llm::Conversation conv(ccfg);
    std::cout << conv.chat(user_input) << "\n";
}
```

---

## Installation

Each library is a single `.hpp` file. Copy what you need:

```bash
# Network libraries (require libcurl)
curl -O https://raw.githubusercontent.com/Mattbusel/llm-stream/main/include/llm_stream.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-embed/main/include/llm_embed.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-agent/main/include/llm_agent.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-rag/main/include/llm_rag.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-eval/main/include/llm_eval.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-chat/main/include/llm_chat.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-vision/main/include/llm_vision.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-batch/main/include/llm_batch.hpp

# Pure C++17 — zero external deps
curl -O https://raw.githubusercontent.com/Mattbusel/llm-cache/main/include/llm_cache.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-cost/main/include/llm_cost.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-retry/main/include/llm_retry.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-format/main/include/llm_format.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-pool/main/include/llm_pool.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-log/main/include/llm_log.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-template/main/include/llm_template.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-mock/main/include/llm_mock.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-router/main/include/llm_router.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-guard/main/include/llm_guard.hpp
curl -O https://raw.githubusercontent.com/Mattbusel/llm-compress/main/include/llm_compress.hpp
```

In exactly **one** `.cpp` file per library, define the implementation macro before including:

```cpp
#define LLM_STREAM_IMPLEMENTATION
#define LLM_RETRY_IMPLEMENTATION
#define LLM_LOG_IMPLEMENTATION
#include "llm_stream.hpp"
#include "llm_retry.hpp"
#include "llm_log.hpp"
```

All other translation units just `#include` without the macro.

---

## Requirements

| Requirement | Detail |
|-------------|--------|
| C++ standard | C++17 or later |
| Compiler | GCC, Clang, MSVC — all supported |
| External deps | libcurl for network libraries (see table above). All others: zero deps. |
| Build system | Any. Works with CMake, Make, Bazel, MSVC, plain `g++`. |

---

## License

All 19 libraries: MIT — Copyright (c) 2026 Mattbusel.
