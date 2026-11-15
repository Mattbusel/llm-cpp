#define LLM_MOCK_IMPLEMENTATION
#include "llm_mock.hpp"
#include <iostream>

// Simulate errors by wrapping MockLLM with a call counter
int main() {
    llm::ScriptedResponses scripted;
    scripted.responses = {"OK", "OK", "OK", "OK"};

    llm::MockConfig cfg;
    cfg.strategy   = scripted;
    cfg.latency_ms = 10.0;

    llm::MockLLM mock(cfg);

    for (int i = 1; i <= 4; ++i) {
        // Simulate error on 2nd call
        if (i == 2) {
            std::cout << "Call " << i << " FAILED: Rate limit exceeded (simulated)\n";
            continue;
        }
        auto resp = mock.complete("test prompt");
        std::cout << "Call " << i << ": " << resp << "\n";
    }

    std::cout << "\nActual LLM calls made: " << mock.call_count() << "\n";
}
