#define LLM_RETRY_IMPLEMENTATION
#include "llm_retry.hpp"
#include <iostream>
#include <stdexcept>

// Simulate a flaky API call that fails twice then succeeds
static int call_count = 0;
std::string fake_llm_call() {
    ++call_count;
    if (call_count < 3) {
        throw llm::LLMError{429, "Rate limited", true};
    }
    return "Hello from the LLM!";
}

int main() {
    llm::RetryConfig cfg;
    cfg.max_attempts    = 5;
    cfg.base_delay_ms   = 100.0;   // fast for demo
    cfg.max_delay_ms    = 2000.0;

    std::cout << "Calling fake LLM with retry (will fail twice)...\n";

    try {
        auto result = llm::with_retry<std::string>(fake_llm_call, cfg);
        std::cout << "Success after " << result.attempts_used << " attempts ("
                  << result.total_elapsed_ms << " ms)\n";
        std::cout << "Response: " << result.value << "\n";
    } catch (const llm::LLMError& e) {
        std::cerr << "Failed: " << e.message << " (HTTP " << e.http_code << ")\n";
        return 1;
    }

    return 0;
}
