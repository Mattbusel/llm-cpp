// model_comparison.cpp: Compare two different models on the same prompt.
#define LLM_AB_IMPLEMENTATION
#include "llm_ab.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    llm::Variant mini;
    mini.name    = "gpt-4o-mini";
    mini.prompt  = "Solve this coding problem: {input}";
    mini.model   = "gpt-4o-mini";
    mini.api_key = key;

    llm::Variant gpt4;
    gpt4.name    = "gpt-4o";
    gpt4.prompt  = "Solve this coding problem: {input}";
    gpt4.model   = "gpt-4o";
    gpt4.api_key = key;

    std::vector<llm::ABSample> samples = {
        {"Write a function to reverse a string in Python.", ""},
        {"Write a function to check if a number is prime.", ""},
        {"Write a function to find the nth Fibonacci number.", ""},
    };

    llm::ABConfig cfg;
    cfg.api_key = key;
    // Score by presence of code block (heuristic)
    cfg.scorer = [](const std::string& out, const std::string&, const std::string&) -> double {
        return (out.find("def ") != std::string::npos ||
                out.find("```") != std::string::npos) ? 1.0 : 0.5;
    };

    // Run each variant individually first
    auto mini_res = llm::run_variant(mini, samples, cfg);
    auto gpt4_res = llm::run_variant(gpt4, samples, cfg);

    std::printf("=== Individual Results ===\n");
    std::printf("%-15s  mean=%.3f  std=%.3f\n", mini_res.name.c_str(),
                mini_res.mean_score, mini_res.std_dev);
    std::printf("%-15s  mean=%.3f  std=%.3f\n", gpt4_res.name.c_str(),
                gpt4_res.mean_score, gpt4_res.std_dev);

    // Then run the full A/B comparison
    auto result = llm::run_ab_test(mini, gpt4, samples, cfg);
    std::printf("\n=== A/B Test ===\n%s\n", result.summary.c_str());
    return 0;
}
