#define LLM_GUARD_IMPLEMENTATION
#include "llm_guard.hpp"
#include <iostream>

int main() {
    // Simulate a response that leaks an API key
    std::string response =
        "Sure! Use this key: sk-abc123XYZ789abcdefghij1234567890 to access the API. "
        "Call us at 555-123-4567 or email support@example.com for help.";

    std::cout << "Raw response:\n  " << response << "\n\n";

    auto result = llm::scan(response);

    std::cout << "Secrets/PII found: " << result.matches.size() << "\n";
    for (const auto& m : result.matches) {
        const char* type = "Unknown";
        switch (m.type) {
            case llm::PiiType::ApiKey:     type = "API KEY";     break;
            case llm::PiiType::Email:      type = "Email";       break;
            case llm::PiiType::Phone:      type = "Phone";       break;
            case llm::PiiType::SSN:        type = "SSN";         break;
            case llm::PiiType::CreditCard: type = "Credit Card"; break;
        }
        std::cout << "  [" << type << "] " << m.value << "\n";
    }

    std::cout << "\nSanitized (safe to return to user):\n  " << result.scrubbed << "\n";
    return 0;
}
