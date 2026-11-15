#define LLM_ROUTER_IMPLEMENTATION
#include "llm_router.hpp"
#include <iostream>

int main() {
    llm::RouterConfig cfg;
    cfg.strategy = llm::RoutingStrategy::Balanced;
    cfg.models = {
        {"gpt-4o-mini",  0.15, 0.5, 0.7, 40},
        {"gpt-4o",       5.0,  1.0, 0.9, 80},
        {"gpt-4-turbo",  10.0, 1.5, 1.0, 100},
    };

    llm::Router router(cfg);

    std::vector<std::string> prompts = {
        "Hi",
        "What is 2+2?",
        "Explain quantum entanglement in detail with examples.",
        "Implement a red-black tree in C++ with all edge cases handled, "
         "including deletion. Show step by step reasoning and code.",
    };

    std::cout << "Routing decisions (Balanced strategy):\n\n";
    for (auto& p : prompts) {
        auto decision = router.route(p);
        std::cout << "Prompt: \"" << p.substr(0, 60)
                  << (p.size() > 60 ? "..." : "") << "\"\n";
        std::cout << "  -> " << decision.model_name
                  << " (" << decision.reason << ")\n\n";
    }
}
