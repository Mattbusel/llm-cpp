#define LLM_BATCH_IMPLEMENTATION
#include "llm_batch.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    const std::string checkpoint = "batch_checkpoint.jsonl";

    // Remove any stale checkpoint
    std::remove(checkpoint.c_str());

    llm::BatchConfig cfg;
    cfg.api_key         = key;
    cfg.model           = "gpt-4o-mini";
    cfg.max_tokens      = 20;
    cfg.num_threads     = 1;
    cfg.checkpoint_path = checkpoint;
    cfg.verbose         = true;

    std::vector<llm::BatchItem> items = {
        {"r1", "Capital of Germany? One word.",  ""},
        {"r2", "Capital of Italy? One word.",    ""},
        {"r3", "Capital of Spain? One word.",    ""},
        {"r4", "Capital of Poland? One word.",   ""},
        {"r5", "Capital of Sweden? One word.",   ""},
    };

    std::cout << "=== Run 1: process first 2 items, then simulate crash ===\n";
    // Only process first 2 items by running a subset
    std::vector<llm::BatchItem> subset(items.begin(), items.begin() + 2);
    auto r1 = llm::process_batch(subset, cfg);
    std::cout << "Completed " << r1.size() << " items, checkpoint saved.\n\n";

    std::cout << "=== Run 2: resume with checkpoint (should skip r1 and r2) ===\n";
    // Now process all 5 with same checkpoint — r1 and r2 should be skipped
    auto r2 = llm::process_batch(items, cfg);

    size_t skipped = 0;
    for (const auto& r : r2) {
        if (r.response == "[skipped - checkpoint]") ++skipped;
        std::cout << "[" << r.id << "] "
                  << (r.response == "[skipped - checkpoint]" ? "SKIPPED" : "PROCESSED")
                  << "\n";
    }

    std::cout << "\nSkipped (from checkpoint): " << skipped << "\n";
    std::cout << "Newly processed: " << (r2.size() - skipped) << "\n";

    // Clean up
    std::remove(checkpoint.c_str());
    return 0;
}
