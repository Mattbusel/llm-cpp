#define LLM_ROUTER_IMPLEMENTATION
#include "llm_router.hpp"
#include <iomanip>
#include <iostream>

int main() {
    llm::RouterConfig cfg;
    cfg.strategy            = llm::RoutingStrategy::Balanced;
    cfg.learn_from_outcomes = true;
    cfg.models = {
        {"gpt-4o-mini",  0.15, 0.5, 0.7, 60},
        {"gpt-4o",       2.50, 1.0, 0.9, 100},
        {"gpt-3.5-turbo", 0.50, 0.6, 0.6, 50},
    };

    llm::Router router(cfg);

    std::vector<std::string> prompts = {
        "Hi", "2+2?", "Hello", "What day is it?", "Ok",
        "Explain quantum computing in depth with examples and math.",
        "Implement Dijkstra's shortest path algorithm in C++ with unit tests.",
        "Compare microservices vs monolith architectures with trade-offs.",
        "Derive the Fourier transform from first principles.",
        "Design a distributed rate limiter for a global API gateway.",
    };

    // Route all prompts and record synthetic outcomes
    for (size_t i = 0; i < prompts.size(); ++i) {
        auto dec = router.route(prompts[i]);
        double quality = (i < 5) ? 0.95 : 0.85; // simple prompts score higher
        router.record_outcome(dec.model_name, quality);
    }

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "=== Routing Stats ===\n\n";

    // We can only call route() — Router doesn't expose internal stats
    // So we track here manually for display
    std::map<std::string, size_t> call_counts;
    for (const auto& p : prompts) {
        auto dec = router.route(p);
        call_counts[dec.model_name]++;
    }

    std::cout << "Model usage across " << prompts.size() << " prompts:\n";
    for (const auto& [model, count] : call_counts) {
        std::cout << "  " << std::setw(16) << std::left << model
                  << ": " << count << " calls\n";
    }

    std::cout << "\nEffective quality after learning:\n";
    for (const auto& m : cfg.models) {
        std::cout << "  " << std::setw(16) << std::left << m.name
                  << ": " << router.effective_quality(m.name) << "\n";
    }

    std::cout << "\nComplexity scores:\n";
    for (const auto& p : prompts) {
        int cx = llm::score_complexity(p);
        std::cout << "  [" << std::setw(3) << cx << "] "
                  << p.substr(0, 50) << "\n";
    }
    return 0;
}
