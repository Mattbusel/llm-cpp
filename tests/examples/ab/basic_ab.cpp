#define LLM_AB_IMPLEMENTATION
#include "llm_ab.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    llm::Variant control;
    control.name        = "verbose";
    control.api_key     = key;
    control.model       = "gpt-4o-mini";
    control.prompt      = "Is the following text positive or negative? {input}";
    control.temperature = 0.0;

    llm::Variant treatment;
    treatment.name        = "concise";
    treatment.api_key     = key;
    treatment.model       = "gpt-4o-mini";
    treatment.prompt      = "Reply with only POSITIVE or NEGATIVE. {input}";
    treatment.temperature = 0.0;

    std::vector<llm::ABSample> samples = {
        {"I love this product!", "positive"},
        {"This is terrible.", "negative"},
        {"Works exactly as described.", "positive"},
        {"Would not recommend.", "negative"},
        {"Absolutely fantastic.", "positive"},
    };

    llm::ABConfig cfg;
    cfg.api_key = key;
    cfg.alpha   = 0.05;

    std::printf("Running A/B test (%zu samples)...\n\n", samples.size());
    auto result = llm::run_ab_test(control, treatment, samples, cfg);

    std::printf("Control   [%s]: mean=%.3f std=%.3f\n",
                result.control.name.c_str(), result.control.mean_score, result.control.std_dev);
    std::printf("Treatment [%s]: mean=%.3f std=%.3f\n",
                result.treatment.name.c_str(), result.treatment.mean_score, result.treatment.std_dev);
    std::printf("p=%.4f  d=%.3f  significant=%s\n",
                result.p_value, result.cohens_d, result.significant ? "yes" : "no");
    std::printf("Winner: %s\nSummary: %s\n", result.winner.c_str(), result.summary.c_str());
    return 0;
}
