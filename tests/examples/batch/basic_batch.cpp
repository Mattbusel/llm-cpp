#define LLM_BATCH_IMPLEMENTATION
#include "llm_batch.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::BatchConfig cfg;
    cfg.api_key        = key;
    cfg.model          = "gpt-4o-mini";
    cfg.max_tokens     = 30;
    cfg.num_threads    = 2;
    cfg.rate_limit_rps = 3.0;
    cfg.verbose        = true;

    std::vector<llm::BatchItem> items = {
        {"q1", "What is the capital of France? One word.",   ""},
        {"q2", "What is 2+2? One number.",                   ""},
        {"q3", "Name one planet. One word.",                 ""},
        {"q4", "What color is the sky? One word.",           ""},
        {"q5", "Boiling point of water in Celsius? One number.", ""},
    };

    std::cout << "Processing " << items.size() << " prompts...\n\n";

    auto results = llm::process_batch(items, cfg);

    std::cout << "\n=== Results ===\n";
    size_t ok = 0;
    for (const auto& r : results) {
        std::cout << "[" << r.id << "] " << (r.success ? "OK" : "FAIL")
                  << " | " << static_cast<int>(r.latency_ms) << "ms"
                  << " | " << r.response.substr(0, 50) << "\n";
        if (r.success) ++ok;
    }

    std::cout << "\nSucceeded: " << ok << "/" << results.size() << "\n";
    return 0;
}
