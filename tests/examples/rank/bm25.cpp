#define LLM_RANK_IMPLEMENTATION
#include "llm_rank.hpp"
#include <iostream>

int main() {
    std::vector<std::string> corpus = {
        "deep learning neural networks artificial intelligence",
        "machine learning gradient descent optimization",
        "natural language processing transformers BERT GPT",
        "computer vision image classification convolutional networks",
        "reinforcement learning reward policy agent",
    };

    llm::BM25 bm25(corpus);

    std::vector<std::string> queries = {
        "neural networks",
        "language models",
        "image recognition",
    };

    for (const auto& q : queries) {
        std::cout << "Query: \"" << q << "\"\n";
        auto s = bm25.scores(q);
        for (size_t i = 0; i < s.size(); ++i)
            printf("  doc %zu: %.4f  %s\n", i, s[i], corpus[i].substr(0, 40).c_str());
        std::cout << "\n";
    }
    return 0;
}
