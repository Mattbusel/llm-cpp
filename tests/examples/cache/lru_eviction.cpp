#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"
#include <iostream>

int main() {
    llm::CacheConfig cfg;
    cfg.max_entries = 3;   // tiny cache to demonstrate eviction
    cfg.ttl_seconds = 0;   // no expiry

    llm::ResponseCache cache(cfg);

    cache.put("a", "alpha");
    cache.put("b", "beta");
    cache.put("c", "gamma");

    std::cout << "Size after 3 puts: " << cache.size() << "\n";

    // Access "a" to make it recently used
    cache.get("a");

    // Adding "d" should evict "b" (LRU)
    cache.put("d", "delta");

    std::cout << "After adding 'd' (evicts LRU):\n";
    std::cout << "  a: " << (cache.get("a") ? *cache.get("a") : "EVICTED") << "\n";
    std::cout << "  b: " << (cache.get("b") ? *cache.get("b") : "EVICTED") << "\n";
    std::cout << "  c: " << (cache.get("c") ? *cache.get("c") : "EVICTED") << "\n";
    std::cout << "  d: " << (cache.get("d") ? *cache.get("d") : "EVICTED") << "\n";

    auto stats = cache.stats();
    std::cout << "\nEvictions: " << stats.evictions << "\n";

    return 0;
}
