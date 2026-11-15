#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    auto jobs = llm::list_jobs(key, 10);
    std::printf("Fine-tune jobs (%zu):\n", jobs.size());
    for (auto& j : jobs) {
        std::printf("  %s  [%s]  model=%s\n",
                    j.id.c_str(), j.status.c_str(), j.model.c_str());
        if (!j.fine_tuned_model.empty())
            std::printf("    -> %s\n", j.fine_tuned_model.c_str());
    }

    // Show how to cancel a running job
    for (auto& j : jobs) {
        if (j.status == "running" || j.status == "queued") {
            std::printf("\nCancelling job %s...\n", j.id.c_str());
            auto cancelled = llm::cancel_job(j.id, key);
            std::printf("New status: %s\n", cancelled.status.c_str());
            break;
        }
    }
    return 0;
}
