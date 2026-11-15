#define LLM_CACHE_IMPLEMENTATION
#include <llm/llm_cache.hpp>
#define LLM_JSON_IMPLEMENTATION
#include <llm/llm_json.hpp>
#ifdef CONSUMER_WITH_CURL
#define LLM_STREAM_IMPLEMENTATION
#include <llm/llm_stream.hpp>
#endif

#include <cstdio>
#include <string>

int main() {
    llm::ResponseCache cache(llm::CacheConfig{});
    int calls = 0;
    for (int i = 0; i < 3; ++i)
        cache.get_or_compute("same prompt", [&] { ++calls; return std::string("answer"); });
    std::printf("model calls: %d (expected 1)\n", calls);
    return calls == 1 ? 0 : 1;
}
