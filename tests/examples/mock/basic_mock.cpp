#define LLM_MOCK_IMPLEMENTATION
#include "llm_mock.hpp"
#include <iostream>

int main() {
    llm::ScriptedResponses scripted;
    scripted.responses = {"Hello!", "How can I help?", "Goodbye!"};

    llm::MockConfig cfg;
    cfg.strategy = scripted;

    llm::MockLLM mock(cfg);

    for (int i = 0; i < 4; ++i) {
        std::string resp = mock.complete("Hi there");
        std::cout << "Call " << (i+1) << ": " << resp << "\n";
    }

    std::cout << "\nTotal calls: " << mock.call_count() << "\n";
    std::cout << "History:\n";
    for (auto& c : mock.call_log())
        std::cout << "  prompt='" << c.prompt << "' response='" << c.response << "'\n";
}
