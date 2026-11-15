#define LLM_GUARD_IMPLEMENTATION
#include "llm_guard.hpp"
#include <iostream>
#include <string>

// Simulate an LLM call (no real API needed for this demo)
static std::string fake_llm(const std::string& prompt) {
    return "I processed your request: " + prompt.substr(0, 40) +
           "... My contact is bot@example.com";
}

int main() {
    std::string user_input =
        "My email is alice@corp.com and SSN is 987-65-4321. "
        "Can you help me reset my password?";

    std::cout << "=== Input Guard ===\n";
    std::cout << "Raw input: " << user_input << "\n\n";

    auto in_result = llm::scan(user_input);
    std::cout << "PII detected: " << in_result.matches.size() << " items\n";
    std::cout << "Injection score: " << in_result.injection_score << "\n";
    std::cout << "Sanitized input: " << in_result.scrubbed << "\n\n";

    // Send sanitized input to LLM
    std::string llm_response = fake_llm(in_result.scrubbed);

    std::cout << "=== Output Guard ===\n";
    std::cout << "Raw response: " << llm_response << "\n\n";

    auto out_result = llm::scan(llm_response);
    std::cout << "PII in response: " << out_result.matches.size() << " items\n";
    std::cout << "Sanitized response: " << out_result.scrubbed << "\n";
    return 0;
}
