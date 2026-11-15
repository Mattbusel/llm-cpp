#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    std::cout << "Listing fine-tuned models...\n\n";
    auto models = llm::list_models(key);

    if (models.empty()) {
        std::cout << "No fine-tuned models found.\n";
        return 0;
    }

    for (size_t i = 0; i < models.size(); ++i) {
        const auto& m = models[i];
        std::cout << "[" << i << "] " << m.id << "\n";
    }

    // Optionally delete a model by ID (pass as argv[1])
    if (argc > 1) {
        std::string model_id = argv[1];
        std::cout << "\nDeleting model: " << model_id << "\n";
        llm::delete_model(model_id, key);
        std::cout << "Deleted.\n";
    } else {
        std::cout << "\nTo delete a model: manage_models <model_id>\n";
    }
    return 0;
}
