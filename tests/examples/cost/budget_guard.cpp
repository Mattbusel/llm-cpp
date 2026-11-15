#define LLM_COST_IMPLEMENTATION
#include "llm_cost.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    using namespace llm;

    // --- Scenario 1: prompt within budget ---
    std::string cheap_prompt = "What is 2 + 2?";
    TokenCount tc_cheap = count(cheap_prompt, models::GPT4O_MINI);

    std::cout << "=== Scenario 1: Cheap prompt ===\n";
    std::cout << "Prompt:     \"" << cheap_prompt << "\"\n";
    std::cout << "Tokens:     " << tc_cheap.tokens << "\n";
    std::cout << "Est. cost:  " << format_cost(tc_cheap.estimated_cost_usd) << "\n";
    std::cout << "Budget:     $0.01\n";

    try {
        assert_budget(tc_cheap, 0.01);
        std::cout << "Result:     ALLOWED, within budget\n\n";
    } catch (const std::runtime_error& e) {
        std::cout << "Result:     BLOCKED: " << e.what() << "\n\n";
    }

    // --- Scenario 2: huge prompt that blows the budget ---
    std::string expensive_prompt(50000, 'A'); // 50k chars of text
    expensive_prompt += " Please analyse all of the above and produce a comprehensive report.";
    TokenCount tc_expensive = count(expensive_prompt, models::CLAUDE_OPUS);

    std::cout << "=== Scenario 2: Expensive prompt (50k chars, Claude Opus) ===\n";
    std::cout << "Characters: " << tc_expensive.characters << "\n";
    std::cout << "Tokens:     " << tc_expensive.tokens     << "\n";
    std::cout << "Est. cost:  " << format_cost(tc_expensive.estimated_cost_usd) << "\n";
    std::cout << "Budget:     $0.05\n";

    try {
        assert_budget(tc_expensive, 0.05);
        std::cout << "Result:     ALLOWED\n\n";
    } catch (const std::runtime_error& e) {
        std::cout << "Result:     BLOCKED: " << e.what() << "\n\n";
    }

    // --- Scenario 3: multi-turn chat budget check ---
    std::vector<std::pair<std::string, std::string>> messages = {
        {"system",    "You are a helpful assistant."},
        {"user",      "Explain quantum computing in detail."},
        {"assistant", "Quantum computing leverages quantum mechanical phenomena..."},
        {"user",      "Now explain how it differs from classical computing with examples."},
    };

    TokenCount tc_chat = count_messages(messages, models::GPT4O);

    std::cout << "=== Scenario 3: Multi-turn chat (GPT-4o) ===\n";
    std::cout << "Messages:   " << messages.size() << "\n";
    std::cout << "Tokens:     " << tc_chat.tokens  << "\n";
    std::cout << "Est. cost:  " << format_cost(tc_chat.estimated_cost_usd) << "\n";
    std::cout << "Budget:     $0.001\n";

    try {
        assert_budget(tc_chat, 0.001);
        std::cout << "Result:     ALLOWED\n";
    } catch (const std::runtime_error& e) {
        std::cout << "Result:     BLOCKED: " << e.what() << "\n";
    }

    return 0;
}
