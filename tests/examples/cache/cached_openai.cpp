// cached_openai.cpp — llm-cache + llm-stream integration example.
//
// Shows how to wrap any LLM call with llm-cache so repeated prompts
// skip the network entirely. Copy llm_stream.hpp from the llm-stream
// repo alongside llm_cache.hpp to compile this example.
//
// Build:
//   cmake -B build && cmake --build build
//   export OPENAI_API_KEY=sk-...
//   ./build/cached_openai

#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"

// If llm_stream.hpp is available alongside llm_cache.hpp, include it.
// Otherwise this example shows the integration pattern with a stub.
#if __has_include("llm_stream.hpp")
    #define LLM_STREAM_IMPLEMENTATION
    #include "llm_stream.hpp"
    #define HAS_LLM_STREAM 1
#else
    #define HAS_LLM_STREAM 0
#endif

#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    llm::CacheConfig cfg;
    cfg.cache_dir            = ".llm_cache_openai";
    cfg.similarity_threshold = 0.90;
    cfg.ttl_seconds          = 86400; // 24-hour TTL

    llm::Cache cache(cfg);

#if HAS_LLM_STREAM
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || key[0] == '\0') {
        std::cerr << "Error: OPENAI_API_KEY not set\n";
        return 1;
    }

    llm::Config stream_cfg;
    stream_cfg.api_key = key;
    stream_cfg.model   = "gpt-4o-mini";

    // LLM call wrapped by cache
    auto openai_call = [&](const std::string& prompt) -> std::string {
        std::string result;
        llm::stream_openai(prompt, stream_cfg,
            [&](std::string_view tok) { result += tok; });
        return result;
    };

    std::string prompt = "Explain what memoization is in one sentence.";

    std::cout << "First call (expect API request):\n";
    auto r1 = llm::cached_call(cache, prompt, openai_call);
    std::cout << r1 << "\n\n";

    std::cout << "Second call (expect cache hit, no API request):\n";
    auto r2 = llm::cached_call(cache, prompt, openai_call);
    std::cout << r2 << "\n\n";

#else
    // Stub demo when llm_stream.hpp is not present
    std::cout << "llm_stream.hpp not found — running stub demo.\n"
              << "Copy llm_stream.hpp from github.com/Mattbusel/llm-stream "
              << "into the include/ directory to use the real OpenAI integration.\n\n";

    auto stub_llm = [](const std::string& p) -> std::string {
        return "[stub response to: " + p + "]";
    };

    auto r1 = llm::cached_call(cache, "Hello, world!", stub_llm);
    std::cout << "Miss -> " << r1 << "\n";
    auto r2 = llm::cached_call(cache, "Hello, world!", stub_llm);
    std::cout << "Hit  -> " << r2 << "\n";
#endif

    auto s = cache.stats();
    std::cout << "Cache: " << s.hits << " hits / " << s.misses << " misses / "
              << s.bytes_saved << " bytes saved\n";

    return 0;
}
