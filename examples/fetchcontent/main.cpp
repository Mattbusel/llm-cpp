// Offline, so it runs in CI: price a prompt before sending it, refuse it if it
// is over budget, and read a field out of a model's JSON reply.
#define LLM_COST_IMPLEMENTATION
#include <llm/llm_cost.hpp>
#define LLM_JSON_IMPLEMENTATION
#include <llm/llm_json.hpp>

#include <cstdio>
#include <exception>

int main() {
    const llm::TokenCount tc =
        llm::count("Summarise this support ticket in one line.", llm::models::GPT4O_MINI);
    std::printf("%zu tokens, %s\n", tc.tokens, llm::format_cost(tc.estimated_cost_usd).c_str());
    try {
        llm::assert_budget(tc, 0.01);  // throws when the prompt would cost more
    } catch (const std::exception& e) {
        std::printf("blocked: %s\n", e.what());
        return 1;
    }

    const llm::json::ParseResult r = llm::json::try_parse(R"({"summary":"Refund issued"})");
    if (!r.ok) return 1;
    std::printf("summary: %s\n", r.value["summary"].as_string().c_str());
    return 0;
}
