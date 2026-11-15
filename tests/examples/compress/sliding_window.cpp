#define LLM_COMPRESS_IMPLEMENTATION
#include "llm_compress.hpp"
#include <iostream>

int main() {
    std::vector<llm::CompressMessage> messages;
    messages.push_back({"system", "You are a helpful assistant."});

    // Simulate a long conversation
    for (int i = 1; i <= 8; ++i) {
        messages.push_back({"user",      "Turn " + std::to_string(i) + " question"});
        messages.push_back({"assistant", "Turn " + std::to_string(i) + " answer — with some detail to fill tokens"});
    }

    std::cout << "Full conversation: " << messages.size() << " messages\n\n";

    // Sliding window: keep last 3 turns (= 6 non-system messages + system)
    llm::CompressConfig cfg;
    cfg.strategy     = llm::SlidingWindow{3};
    cfg.token_budget = 999999; // no token cap, just window size

    auto result = llm::compress_messages(messages, cfg);

    std::cout << "After sliding window (max_turns=3):\n";
    for (auto& m : result.messages)
        std::cout << "  [" << m.role << "] " << m.content << "\n";

    std::cout << "\nRemoved " << result.messages_removed << " messages\n";
}
