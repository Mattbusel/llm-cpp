#define LLM_ROUTER_IMPLEMENTATION
#include "llm_router.hpp"
#include <iostream>

int main() {
    llm::RouterConfig cfg;
    cfg.strategy = llm::RoutingStrategy::Balanced;
    cfg.models = {
        {"gpt-4o-mini",  0.15, 0.5, 0.7, 60},
        {"gpt-4o",       2.50, 1.0, 1.0, 100},
        {"gpt-3.5-turbo", 0.50, 0.6, 0.6, 50},
    };
    cfg.learn_from_outcomes = true;

    llm::Router router(cfg);

    std::vector<std::string> prompts = {
        "Hi",
        "What is 2+2?",
        "Explain the difference between TCP and UDP in detail, with examples and a comparison table.",
        "Write a Python implementation of merge sort with full docstrings, type hints, and unit tests.",
        "```python\ndef foo(): pass\n``` Please refactor this function to use async/await and add error handling.",
    };

    for (const auto& p : prompts) {
        auto dec = router.route(p);
        std::cout << "Prompt: \"" << p.substr(0, 50) << (p.size() > 50 ? "..." : "") << "\"\n"
                  << "  -> " << dec.model_name << " (" << dec.reason << ")\n\n";
    }

    // Simulate outcome learning
    router.record_outcome("gpt-4o-mini", 0.4); // poor quality for complex task
    router.record_outcome("gpt-4o",      0.95);
    std::cout << "After learning:\n";
    std::cout << "  gpt-4o-mini effective quality: " << router.effective_quality("gpt-4o-mini") << "\n";
    std::cout << "  gpt-4o       effective quality: " << router.effective_quality("gpt-4o") << "\n";

    // Re-route complex prompt after learning
    auto dec2 = router.route(prompts.back());
    std::cout << "Complex prompt re-routed to: " << dec2.model_name << "\n";

    return 0;
}
