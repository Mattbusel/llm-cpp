#define LLM_EVAL_IMPLEMENTATION
#include "llm_eval.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !key[0]) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }

    llm::EvalConfig cfg_a;
    cfg_a.api_key = key; cfg_a.model = "gpt-4o-mini"; cfg_a.runs = 3;

    llm::EvalConfig cfg_b;
    cfg_b.api_key = key; cfg_b.model = "gpt-4o-mini"; cfg_b.runs = 3;
    cfg_b.temperature = 0.0;

    std::string prompt = "What is the capital of France?";
    std::cout << "Comparing temperature=0.7 vs temperature=0.0\n\n";

    auto cr = llm::compare(prompt, cfg_a, cfg_b);
    std::cout << "A: " << cr.model_a.summary << "\n";
    std::cout << "B: " << cr.model_b.summary << "\n";
    std::cout << "Winner: " << cr.winner << "\n";
    return 0;
}
