#define LLM_COMPRESS_IMPLEMENTATION
#include "llm_compress.hpp"
#include <iostream>

int main() {
    std::vector<llm::CompressMessage> history = {
        {"system",    "You are a helpful assistant."},
        {"user",      "Tell me about the history of the Roman Empire."},
        {"assistant", "The Roman Empire was a vast civilization that spanned centuries..."},
        {"user",      "What caused its fall?"},
        {"assistant", "Multiple factors contributed to the fall including economic troubles..."},
        {"user",      "Who were the last emperors?"},
        {"assistant", "The last emperors of the Western Roman Empire included Romulus Augustulus..."},
        {"user",      "What year did it fall?"},
        {"assistant", "The Western Roman Empire fell in 476 AD when Odoacer deposed Romulus Augustulus."},
        {"user",      "And the Eastern Empire?"},
        {"assistant", "The Eastern Roman Empire, also known as the Byzantine Empire, lasted until 1453 AD."},
    };

    std::cout << "Tokens before: " << llm::estimate_tokens(history) << "\n\n";

    // TruncateHead
    {
        llm::CompressConfig cfg;
        cfg.strategy     = llm::TruncateHead{};
        cfg.token_budget = 100;
        auto r = llm::compress_messages(history, cfg);
        std::cout << "TruncateHead: " << r.tokens_before << " -> " << r.tokens_after
                  << " tokens, removed " << r.messages_removed << " messages\n";
    }

    // SlidingWindow
    {
        llm::CompressConfig cfg;
        cfg.strategy     = llm::SlidingWindow{3};
        cfg.token_budget = 99999;
        auto r = llm::compress_messages(history, cfg);
        std::cout << "SlidingWindow(3): kept " << r.messages.size() << " messages\n";
    }

    // TruncateSmart
    {
        llm::CompressConfig cfg;
        cfg.strategy     = llm::TruncateSmart{};
        cfg.token_budget = 80;
        auto r = llm::compress_messages(history, cfg);
        std::cout << "TruncateSmart: " << r.tokens_before << " -> " << r.tokens_after
                  << " tokens, removed " << r.messages_removed << " messages\n";
    }

    // Summarize (without curl — shows placeholder)
    {
        llm::CompressConfig cfg;
        cfg.strategy     = llm::Summarize{};
        cfg.token_budget = 80;
        auto r = llm::compress_messages(history, cfg);
        std::cout << "Summarize: kept " << r.messages.size() << " messages, "
                  << "removed " << r.messages_removed << "\n";
        if (!r.messages.empty())
            std::cout << "  First msg role: " << r.messages[0].role << "\n";
    }

    return 0;
}
