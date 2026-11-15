#define LLM_GUARD_IMPLEMENTATION
#include "llm_guard.hpp"
#include <iostream>

int main() {
    std::string text =
        "Contact John at john.doe@example.com or call 555-867-5309. "
        "His SSN is 123-45-6789 and card is 4532015112830366.";

    std::cout << "Original:\n  " << text << "\n\n";

    auto result = llm::scan(text);

    std::cout << "PII found (" << result.matches.size() << " matches):\n";
    for (const auto& m : result.matches) {
        const char* type = "Other";
        switch (m.type) {
            case llm::PiiType::Email:      type = "Email";       break;
            case llm::PiiType::Phone:      type = "Phone";       break;
            case llm::PiiType::SSN:        type = "SSN";         break;
            case llm::PiiType::CreditCard: type = "CreditCard";  break;
            case llm::PiiType::ApiKey:     type = "ApiKey";      break;
        }
        std::cout << "  [" << type << "] \"" << m.value
                  << "\" at offset " << m.offset << "\n";
    }

    std::cout << "\nSanitized:\n  " << result.scrubbed << "\n";
    std::cout << "\nHas PII: " << (result.has_pii ? "yes" : "no") << "\n";

    // Quick scrub
    std::cout << "\nQuick scrub:\n  " << llm::scrub(text) << "\n";
    return 0;
}
