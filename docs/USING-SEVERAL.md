# Using several llm-cpp headers together

Back to the [README](../README.md) or the [reference](REFERENCE.md).


**Give each implementation its own `.cpp` file.** Several headers use the same internal helper names (for example `llm::detail::json_escape`), so defining two `*_IMPLEMENTATION` macros in one translation unit can fail to compile (llm-log with llm-stream is one such pair). In separate translation units they link together fine. As a check, all 26 implementations, each in its own `.cpp`, were compiled and linked into a single binary with MSVC 19.44 and libcurl on 2026-09-25. CI repeats the same check with g++ and libcurl on every push (the `link-all` job in [ci.yml](../.github/workflows/ci.yml)).

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
