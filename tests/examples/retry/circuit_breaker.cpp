#define LLM_RETRY_IMPLEMENTATION
#include "llm_retry.hpp"
#include <iostream>

int main() {
    llm::CircuitBreakerConfig cb_cfg;
    cb_cfg.failure_threshold = 3;
    cb_cfg.timeout_seconds   = 2.0;

    llm::CircuitBreaker breaker("openai", cb_cfg);

    auto always_fails = []() -> std::string {
        throw llm::LLMError{500, "Internal server error", true};
        return "";
    };

    std::cout << "Driving 3 failures to open the circuit...\n";
    for (int i = 0; i < 3; ++i) {
        try { breaker.call<std::string>(always_fails); }
        catch (const llm::LLMError& e) {
            std::cout << "  Attempt " << (i+1) << " failed: " << e.message << "\n";
        }
    }

    std::cout << "Circuit state: "
              << (breaker.state() == llm::CircuitState::Open ? "OPEN" : "closed") << "\n";

    // 4th call should fast-fail (circuit open)
    try {
        breaker.call<std::string>(always_fails);
    } catch (const std::runtime_error& e) {
        std::cout << "Fast-fail: " << e.what() << "\n";
    }

    auto stats = breaker.stats();
    std::cout << "Stats: " << stats.consecutive_failures << " consecutive failures, "
              << "failure rate: " << stats.failure_rate << "\n";

    // Manual reset
    breaker.reset();
    std::cout << "After reset, state: "
              << (breaker.state() == llm::CircuitState::Closed ? "CLOSED" : "other") << "\n";

    return 0;
}
