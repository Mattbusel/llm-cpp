#define LLM_RANK_IMPLEMENTATION
#include "llm_rank.hpp"
#include <cstdio>

int main() {
    std::vector<std::string> corpus = {
        "The quick brown fox jumps over the lazy dog",
        "Machine learning is a subset of artificial intelligence",
        "Natural language processing enables computers to understand text",
        "Deep learning uses neural networks with many layers",
        "The fox ran quickly through the forest",
    };

    llm::BM25 bm25(corpus);
    std::string query = "fox quick";
    auto scores = bm25.scores(query);

    std::printf("BM25 scores for query: \"%s\"\n", query.c_str());
    for (size_t i = 0; i < corpus.size(); ++i)
        std::printf("  %.4f  %s\n", scores[i], corpus[i].c_str());
    return 0;
}
