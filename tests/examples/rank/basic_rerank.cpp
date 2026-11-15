#define LLM_RANK_IMPLEMENTATION
#include "llm_rank.hpp"
#include <cstdio>

int main() {
    std::string query = "machine learning optimization";
    std::vector<std::string> passages = {
        "Gradient descent is a first-order optimization algorithm for machine learning.",
        "The Eiffel Tower is located in Paris, France.",
        "Adam optimizer adapts learning rates for each parameter in neural networks.",
        "Stochastic gradient descent processes mini-batches for faster convergence.",
        "Shakespeare wrote Hamlet in the early 1600s.",
    };

    auto results = llm::rerank_local(query, passages);
    std::printf("Query: %s\n\nRanked passages:\n", query.c_str());
    for (auto& r : results)
        std::printf("  [%d] (%.3f) %s\n", r.new_rank, r.score, r.passage.substr(0, 60).c_str());
    return 0;
}
