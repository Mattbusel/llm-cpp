#define LLM_ROUTER_IMPLEMENTATION
#include "llm_router.hpp"
#include <iostream>

int main() {
    llm::RouterConfig cfg;
    cfg.strategy            = llm::RoutingStrategy::QualityOptimized;
    cfg.learn_from_outcomes = true;
    cfg.ema_alpha           = 0.3; // faster learning for demo
    cfg.models = {
        {"gpt-4o-mini", 0.15, 0.5, 0.7, 60},
        {"gpt-4o",      2.50, 1.0, 0.9, 100},
    };

    llm::Router router(cfg);

    std::string complex_prompt =
        "Implement a lock-free concurrent hash map in C++ with compare-and-swap.";

    std::cout << "Initial routing (quality-optimized):\n";
    auto d1 = router.route(complex_prompt);
    std::cout << "  -> " << d1.model_name << " (" << d1.reason << ")\n\n";

    // Record poor outcome for gpt-4o — maybe it timed out or gave bad answers
    std::cout << "Recording outcomes: gpt-4o=0.2 (poor), gpt-4o-mini=0.9 (great)\n";
    router.record_outcome("gpt-4o",      0.2);
    router.record_outcome("gpt-4o-mini", 0.9);
    router.record_outcome("gpt-4o",      0.15); // consistent poor outcome

    std::cout << "\nEffective quality after learning:\n";
    std::cout << "  gpt-4o-mini: " << router.effective_quality("gpt-4o-mini") << "\n";
    std::cout << "  gpt-4o:      " << router.effective_quality("gpt-4o") << "\n\n";

    std::cout << "Re-routing same prompt:\n";
    auto d2 = router.route(complex_prompt);
    std::cout << "  -> " << d2.model_name << " (" << d2.reason << ")\n";
    return 0;
}
