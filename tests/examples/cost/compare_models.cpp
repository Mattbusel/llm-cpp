#define LLM_COST_IMPLEMENTATION
#include "llm_cost.hpp"

#include <iostream>
#include <iomanip>

static const std::string PROMPT =
    "Summarize the key architectural differences between microservices and "
    "monolithic applications, covering deployment, scalability, fault isolation, "
    "development velocity, and operational complexity. Give concrete examples.";

int main() {
    using namespace llm;

    auto comparisons = compare_costs(PROMPT);

    std::cout << "=== Cost Comparison Table ===\n";
    std::cout << "Prompt: " << PROMPT.substr(0, 60) << "...\n\n";

    std::cout << std::left
              << std::setw(26) << "Model"
              << std::setw(10) << "Tokens"
              << std::setw(14) << "Input Cost"
              << "\n";
    std::cout << std::string(50, '-') << "\n";

    for (const auto& row : comparisons) {
        std::cout << std::left
                  << std::setw(26) << row.model_name
                  << std::setw(10) << row.tokens
                  << std::setw(14) << format_cost(row.input_cost_usd)
                  << "\n";
    }

    std::cout << "\nNote: input cost only. Output cost depends on response length.\n";
    std::cout << "Token counts are estimates (±5% vs tiktoken).\n";

    return 0;
}
