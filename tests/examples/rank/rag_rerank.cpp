#define LLM_RANK_IMPLEMENTATION
#include "llm_rank.hpp"
#include <cstdio>
#include <vector>
#include <string>

// Simulates a RAG pipeline: retrieve candidates, then rerank locally.
// In production, use llm-rag for retrieval + llm-rank for reranking.

static std::vector<std::string> simulate_retrieval(const std::string& query) {
    // Pretend vector search returned these 8 candidates
    (void)query;
    return {
        "Python is a high-level programming language known for readability.",
        "Machine learning models require large amounts of training data.",
        "Neural networks are computational systems inspired by biological brains.",
        "Python has extensive libraries for data science and machine learning.",
        "Deep learning uses many layers of neural networks for complex tasks.",
        "SQL is a language for managing relational databases.",
        "scikit-learn provides tools for machine learning in Python.",
        "JavaScript is primarily used for web development.",
    };
}

int main() {
    std::string query = "Python machine learning libraries";

    // Step 1: retrieval (simulated)
    auto candidates = simulate_retrieval(query);
    std::printf("Query: %s\n", query.c_str());
    std::printf("Retrieved %zu candidates from vector store\n\n", candidates.size());

    // Step 2: rerank
    llm::LocalRankConfig cfg;
    cfg.bm25_weight    = 0.7;
    cfg.keyword_weight = 0.3;

    auto reranked = llm::rerank_local(query, candidates, cfg);

    std::printf("=== After reranking ===\n");
    std::printf("%-5s %-5s %-6s  %s\n", "Rank", "Was", "Score", "Passage (first 60 chars)");
    std::printf("%-5s %-5s %-6s  %s\n", "----", "---", "-----", "------");
    for (auto& r : reranked) {
        std::printf("  %-3d   %-3d  %.3f   %s\n",
            r.new_rank, r.original_rank, r.score,
            r.passage.substr(0, 60).c_str());
    }

    std::printf("\nTop 3 for LLM context:\n");
    for (int i = 0; i < 3 && i < (int)reranked.size(); ++i)
        std::printf("  %d. %s\n", i+1, reranked[i].passage.c_str());
    return 0;
}
