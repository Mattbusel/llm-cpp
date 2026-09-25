#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"
#include <cstdio>

int main() {
    llm::CacheConfig cfg;
    cfg.max_entries = 2;                  // tiny, to show LRU eviction
    llm::ResponseCache cache(cfg);

    int api_calls = 0;
    auto ask = [&](const std::string& prompt) {
        return cache.get_or_compute(prompt, [&] {
            ++api_calls;                  // your real model call goes here
            return "answer #" + std::to_string(api_calls);
        });
    };

    for (const char* p : {"What is RAII?", "what is raii?",
                          "Explain move semantics", "What is SFINAE?",
                          "What is RAII?"})
        std::printf("%-24s -> %s\n", p, ask(p).c_str());

    auto s = cache.stats();
    std::printf("\napi calls %d | hits %zu | misses %zu | evictions %zu\n",
                api_calls, s.hits, s.misses, s.evictions);
}
