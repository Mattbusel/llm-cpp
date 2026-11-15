#define LLM_RANK_IMPLEMENTATION
#include "llm_rank.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    std::string query = "What causes climate change?";
    std::vector<std::string> passages = {
        "Greenhouse gases like CO2 trap heat in the atmosphere, warming the planet.",
        "The stock market fluctuates based on investor sentiment.",
        "Burning fossil fuels releases carbon dioxide, a major driver of climate change.",
        "Deforestation reduces the Earth's capacity to absorb carbon dioxide.",
    };

    llm::LLMRankConfig cfg;
    cfg.api_key = key;
    cfg.model   = "gpt-4o-mini";

    auto results = llm::rerank_llm(query, passages, cfg);
    std::printf("LLM-ranked results:\n");
    for (auto& r : results)
        std::printf("  [%d] score=%.2f  %s\n", r.new_rank, r.score,
                    r.passage.substr(0, 70).c_str());
    return 0;
}
