#define LLM_ROUTER_IMPLEMENTATION
#include "llm_router.hpp"
#include <iostream>

int main() {
    llm::RouterConfig cfg;
    cfg.strategy      = llm::RoutingStrategy::Budget;
    cfg.max_cost_tier = 1; // only models with cost_per_1k <= 1.0
    cfg.models = {
        {"gpt-4o-mini",  0.15, 0.5, 0.7, 40},
        {"gpt-4o",       5.0,  1.0, 0.9, 80},
        {"gpt-4-turbo", 10.0,  1.5, 1.0, 100},
    };

    llm::Router router(cfg);

    std::vector<std::string> prompts = {
        "What is 2+2?",
        "Write a B-tree implementation in C++ with deletion.",
        "Summarize the French Revolution in one sentence.",
        "Implement a distributed consensus algorithm with Byzantine fault tolerance.",
    };

    std::cout << "Budget routing (max cost tier = 1.0, blocks gpt-4o and gpt-4-turbo):\n\n";
    for (const auto& p : prompts) {
        auto dec = router.route(p);
        int  cx  = llm::score_complexity(p);
        std::cout << "Prompt:     \"" << p.substr(0, 55)
                  << (p.size() > 55 ? "..." : "") << "\"\n";
        std::cout << "Complexity: " << cx << "/100\n";
        std::cout << "Routed to:  " << dec.model_name
                  << " (" << dec.reason << ")\n\n";
    }
    return 0;
}
