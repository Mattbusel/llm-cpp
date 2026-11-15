#define LLM_MOCK_IMPLEMENTATION
#include "llm_mock.hpp"
#include <iostream>

int main() {
    llm::PatternResponses pr;
    pr.patterns = {
        {"hello",   "Hello! How can I assist you today?"},
        {"weather", "I don't have real-time weather data."},
        {"france",  "The capital of France is Paris."},
        {"+",       "I can see you want math. The answer is 42."},
    };
    pr.fallback = "I'm not sure how to answer that.";

    llm::MockConfig cfg;
    cfg.strategy = pr;

    llm::MockLLM mock(cfg);

    std::vector<std::string> prompts = {
        "Hi there!",
        "What's the weather like?",
        "What is the capital of france?",
        "What is 5 + 3?",
        "Tell me about quantum physics",
    };

    for (auto& p : prompts)
        std::cout << "Q: " << p << "\nA: " << mock.complete(p) << "\n\n";
}
