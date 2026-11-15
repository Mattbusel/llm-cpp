#define LLM_EVAL_IMPLEMENTATION
#include "llm_eval.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !key[0]) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }

    llm::EvalConfig cfg;
    cfg.api_key     = key;
    cfg.model       = "gpt-4o-mini";
    cfg.runs        = 5;
    cfg.temperature = 0.7;

    std::string prompt = "Explain what a mutex is in one sentence.";
    std::cout << "Evaluating: \"" << prompt << "\" x" << cfg.runs << " runs\n\n";

    auto result = llm::evaluate(prompt, cfg);
    std::cout << result.summary << "\n\n";

    auto score = llm::score_response(
        result.runs[0].response,
        {"mutex", "lock", "thread", "synchronization"}
    );
    std::cout << "Keyword score: " << score.score * 100 << "%\n";
    std::cout << "Found: ";
    for (const auto& w : score.found) std::cout << w << " ";
    std::cout << "\nMissing: ";
    for (const auto& w : score.missing) std::cout << w << " ";
    std::cout << "\n";
    return 0;
}
