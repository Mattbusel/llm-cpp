#define LLM_COST_IMPLEMENTATION
#include "llm_cost.hpp"
#include <cstdio>

int main() {
    std::string prompt;                   // a 12,000-character prompt
    while (prompt.size() < 12000)
        prompt += "Summarise the attached incident report. ";

    for (const auto& row : llm::compare_costs(prompt))
        std::printf("%-18s %5zu tokens  %s\n", row.model_name.c_str(),
                    row.tokens, llm::format_cost(row.input_cost_usd).c_str());

    auto tc = llm::count(prompt, llm::models::CLAUDE_OPUS);
    try {
        llm::assert_budget(tc, 0.01);     // refuse anything over one cent
    } catch (const std::exception& e) {
        std::printf("\nblocked: %s\n", e.what());
    }
}
