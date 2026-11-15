// scored_ab.cpp: Use LLM as a judge to score outputs 0-10.
#define LLM_AB_IMPLEMENTATION
#include "llm_ab.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    llm::Variant brief;
    brief.name    = "brief";
    brief.prompt  = "Answer in one sentence: {input}";
    brief.model   = "gpt-4o-mini";
    brief.api_key = key;

    llm::Variant detailed;
    detailed.name    = "detailed";
    detailed.prompt  = "Give a thorough explanation: {input}";
    detailed.model   = "gpt-4o-mini";
    detailed.api_key = key;

    std::vector<llm::ABSample> samples = {
        {"Why is the sky blue?",          ""},
        {"How does photosynthesis work?", ""},
        {"What is machine learning?",     ""},
    };

    llm::ABConfig cfg;
    cfg.api_key = key;
    // Score by output length as a proxy for thoroughness (0-1 normalized)
    cfg.scorer = [](const std::string& out, const std::string&, const std::string&) -> double {
        double len = (double)out.size();
        return std::min(1.0, len / 500.0);
    };

    auto result = llm::run_ab_test(brief, detailed, samples, cfg);
    std::printf("Brief   mean: %.3f\n", result.control.mean_score);
    std::printf("Detailed mean: %.3f\n", result.treatment.mean_score);
    std::printf("%s\n", result.summary.c_str());
    return 0;
}
