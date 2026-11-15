#define LLM_COST_IMPLEMENTATION
#include "llm_cost.hpp"
#include <cstdio>

int main() {
    std::string prompt;                   // a 12,000-character prompt
    while (prompt.size() < 12000)
        prompt += "Summarise the attached incident report. ";

    // Any API id or alias from the built-in table (generated from llm-prices).
    std::vector<llm::Model> shortlist;
    for (const char* id : {"gpt-6-luna", "gemini-3.1-flash-lite", "deepseek-chat",
                           "claude-haiku-4-5", "mistral-large-latest", "grok-4.3",
                           "gpt-6-sol", "claude-sonnet-5", "claude-opus-4-5", "gpt-6-astra"})
        if (const llm::Model* m = llm::find_model(id))
            shortlist.push_back(*m);

    for (const auto& row : llm::compare_costs(prompt, shortlist))
        std::printf("%-22s %5zu tokens  %s\n", row.model_name.c_str(),
                    row.tokens, llm::format_cost(row.input_cost_usd).c_str());

    auto tc = llm::count(prompt, llm::models::CLAUDE_OPUS);
    try {
        llm::assert_budget(tc, 0.01);     // refuse anything over one cent
    } catch (const std::exception& e) {
        std::printf("\nblocked: %s\n", e.what());
    }
}
