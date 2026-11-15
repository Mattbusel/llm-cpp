#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"
#include <iostream>

int main() {
    llm::CacheConfig cfg;
    cfg.max_entries  = 100;
    cfg.ttl_seconds  = 60.0;
    cfg.case_sensitive = false;

    llm::ResponseCache cache(cfg);

    // Simulate a slow LLM call
    int call_count = 0;
    auto fake_llm = [&](const std::string& prompt) -> std::string {
        return cache.get_or_compute(prompt, [&]() -> std::string {
            ++call_count;
            return "Response to: " + prompt;
        });
    };

    std::string p1 = "What is the capital of France?";
    std::string p2 = "Explain recursion.";

    std::cout << fake_llm(p1) << "\n";   // miss -> compute
    std::cout << fake_llm(p2) << "\n";   // miss -> compute
    std::cout << fake_llm(p1) << "\n";   // hit
    std::cout << fake_llm(p2) << "\n";   // hit

    auto stats = cache.stats();
    std::cout << "\nCache stats:\n"
              << "  Hits:      " << stats.hits    << "\n"
              << "  Misses:    " << stats.misses  << "\n"
              << "  Hit rate:  " << stats.hit_rate() * 100 << "%\n"
              << "  LLM calls: " << call_count    << "\n";

    std::cout << "\nHash of p1: " << llm::prompt_hash(p1) << "\n";

    return 0;
}
