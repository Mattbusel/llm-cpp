#define LLM_BATCH_IMPLEMENTATION
#include "llm_batch.hpp"
#include <cstdlib>
#include <iostream>

// Demonstrates checkpoint/resumability: run twice, 2nd run skips completed items
int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::BatchConfig cfg;
    cfg.api_key         = key;
    cfg.model           = "gpt-4o-mini";
    cfg.max_tokens      = 20;
    cfg.num_threads     = 2;
    cfg.rate_limit_rps  = 3.0;
    cfg.checkpoint_path = "checkpoint.jsonl";
    cfg.verbose         = true;

    cfg.on_progress = [](const llm::BatchResult& r, size_t done, size_t total) {
        std::cout << "Progress: " << done << "/" << total << " - "
                  << r.id << ": " << (r.success ? r.response.substr(0, 30) : r.error) << "\n";
    };

    std::vector<llm::BatchItem> items = {
        {"c1", "One-word country in Europe?", ""},
        {"c2", "One-word ocean name?",         ""},
        {"c3", "One-word planet name?",         ""},
    };

    std::cout << "=== Run 1 ===\n";
    auto results = llm::process_batch(items, cfg);
    std::cout << "Run 1 done. Checkpoint saved.\n\n";

    std::cout << "=== Run 2 (resumes from checkpoint) ===\n";
    auto results2 = llm::process_batch(items, cfg);
    std::cout << "Run 2 done. Skipped previously completed items.\n\n";

    // Print final results
    for (auto& r : results2) {
        std::cout << "[" << r.id << "] " << r.response << "\n";
    }
    return 0;
}
