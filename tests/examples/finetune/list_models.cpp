#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <cstdio>
#include <cstdlib>
#include <ctime>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    auto models = llm::list_models(key);
    std::printf("Fine-tuned models (%zu):\n", models.size());
    for (auto& m : models) {
        char buf[32] = {};
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&m.created_at));
        std::printf("  %s  (created %s)\n", m.id.c_str(), buf);
    }
    if (models.empty()) std::printf("  (none)\n");
    return 0;
}
