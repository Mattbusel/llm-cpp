#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    auto jobs = llm::list_jobs(key, 10);
    std::cout << "Fine-tune jobs (" << jobs.size() << "):\n";
    for (const auto& j : jobs) {
        std::cout << "  [" << j.status << "] " << j.id
                  << " model=" << j.model;
        if (!j.fine_tuned_model.empty())
            std::cout << " -> " << j.fine_tuned_model;
        if (!j.error.empty())
            std::cout << " ERROR=" << j.error;
        std::cout << "\n";
    }

    auto models = llm::list_models(key);
    std::cout << "\nFine-tuned models (" << models.size() << "):\n";
    for (const auto& m : models)
        std::cout << "  " << m.id << "\n";

    return 0;
}
