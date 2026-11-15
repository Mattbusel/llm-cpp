#define LLM_COMPRESS_IMPLEMENTATION
#include "llm_compress.hpp"
#include <iostream>

int main() {
    std::vector<llm::CompressMessage> messages;
    messages.push_back({"system", "You are a helpful assistant."});
    for (int i = 1; i <= 10; ++i) {
        messages.push_back({"user",      "Question " + std::to_string(i) + ": some question about topic " + std::to_string(i)});
        messages.push_back({"assistant", "Answer " + std::to_string(i) + ": detailed response about topic " + std::to_string(i)});
    }

    std::cout << "Original: " << messages.size() << " messages, "
              << llm::estimate_tokens(messages) << " tokens\n\n";

    // Compare strategies
    for (auto& [name, strat] : std::vector<std::pair<std::string, llm::CompressStrategy>>{
        {"TruncateHead",  llm::TruncateHead{}},
        {"TruncateTail",  llm::TruncateTail{}},
        {"TruncateSmart", llm::TruncateSmart{}},
        {"SlidingWindow", llm::SlidingWindow{3}},
    }) {
        llm::CompressConfig cfg;
        cfg.strategy     = strat;
        cfg.token_budget = 80;
        auto r = llm::compress_messages(messages, cfg);
        std::cout << name << ": "
                  << r.messages.size() << " messages, "
                  << r.tokens_after << " tokens ("
                  << r.messages_removed << " removed)\n";
    }
}
