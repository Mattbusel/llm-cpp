#define LLM_COMPRESS_IMPLEMENTATION
#include "llm_compress.hpp"
#include <cstdio>

int main() {
    std::string q;
    for (int i = 0; i < 8; ++i) q += "why is my iterator invalid? ";

    std::vector<llm::CompressMessage> history = {
        {"system", "You are a terse C++ reviewer."}};
    for (int i = 1; i <= 12; ++i) {
        auto n = std::to_string(i);
        history.push_back({"user", "Q" + n + ": " + q});
        history.push_back({"assistant", "A" + n + ": push_back reallocated."});
    }

    llm::CompressConfig cfg;
    cfg.strategy = llm::SlidingWindow{3};     // keep the last 3 turns
    cfg.token_budget = 1000;

    auto r = llm::compress_messages(history, cfg);
    std::printf("tokens %zu -> %zu, dropped %zu of %zu messages\n\n",
                r.tokens_before, r.tokens_after, r.messages_removed,
                history.size());
    for (const auto& m : r.messages)
        std::printf("%-9s %.40s\n", m.role.c_str(), m.content.c_str());
}
