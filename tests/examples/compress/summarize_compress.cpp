#define LLM_COMPRESS_IMPLEMENTATION
#include "llm_compress.hpp"
#include <iostream>

int main() {
    std::vector<llm::CompressMessage> messages;
    messages.push_back({"system", "You are a helpful assistant."});
    for (int i = 1; i <= 6; ++i) {
        messages.push_back({"user",      "Question " + std::to_string(i)});
        messages.push_back({"assistant", "Answer " + std::to_string(i) + " with more detail"});
    }

    std::cout << "Before: " << llm::estimate_tokens(messages) << " tokens\n";

    // Summarize strategy without libcurl (shows placeholder)
    llm::CompressConfig cfg;
    cfg.strategy     = llm::Summarize{"", "gpt-4o-mini"};
    cfg.token_budget = 30;

    auto result = llm::compress_messages(messages, cfg);

    std::cout << "After summarize compress:\n";
    std::cout << "  Messages: " << result.messages.size() << "\n";
    std::cout << "  Tokens: " << result.tokens_after << "\n\n";

    for (auto& m : result.messages)
        std::cout << "  [" << m.role << "] " << m.content << "\n";

    std::cout << "\nNote: set OPENAI_API_KEY and define LLM_COMPRESS_SUMMARIZE\n"
                 "to enable real LLM-based summarization.\n";
}
