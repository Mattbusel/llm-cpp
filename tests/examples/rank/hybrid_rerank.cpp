#define LLM_RANK_IMPLEMENTATION
#include "llm_rank.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    std::string query = "Python async programming";
    std::vector<std::string> passages = {
        "Python's asyncio library enables asynchronous I/O operations.",
        "Java is an object-oriented programming language.",
        "The async/await syntax in Python 3.5+ simplifies coroutine usage.",
        "HTTP requests can be made asynchronously with aiohttp in Python.",
        "JavaScript uses event loops for non-blocking operations.",
        "Python generators yield values lazily on demand.",
    };

    llm::LocalRankConfig local_cfg;
    llm::LLMRankConfig llm_cfg;
    llm_cfg.api_key = key;
    llm_cfg.model   = "gpt-4o-mini";

    // Hybrid: BM25 ranks all, LLM re-ranks top 3
    auto results = llm::rerank_hybrid(query, passages, local_cfg, llm_cfg, 3);
    std::printf("Hybrid reranked results:\n");
    for (auto& r : results)
        std::printf("  [%d] score=%.3f  %s\n", r.new_rank, r.score,
                    r.passage.substr(0, 65).c_str());
    return 0;
}
