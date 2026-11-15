#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <fstream>
#include <iostream>

int main() {
    // Show write_training_jsonl output format — no API call needed
    std::vector<llm::TrainingExample> examples = {
        {"You classify text sentiment. Reply POSITIVE or NEGATIVE.",
         "I love this!", "POSITIVE"},
        {"You classify text sentiment. Reply POSITIVE or NEGATIVE.",
         "Worst purchase ever.", "NEGATIVE"},
        {"You classify text sentiment. Reply POSITIVE or NEGATIVE.",
         "Pretty good value.", "POSITIVE"},
        {"You are a code reviewer. Point out one issue briefly.",
         "int x = 0; x = x / 0;", "Division by zero: will crash at runtime."},
        {"You are a code reviewer. Point out one issue briefly.",
         "char* s = malloc(10);", "Missing null check after malloc."},
    };

    const std::string path = "training_demo.jsonl";
    llm::write_training_jsonl(examples, path);
    std::cout << "Wrote " << examples.size() << " examples to " << path << "\n\n";

    // Print the JSONL so the user can see the format
    std::ifstream f(path);
    std::string line;
    std::cout << "=== JSONL format (each line is one training example) ===\n";
    while (std::getline(f, line))
        std::cout << line.substr(0, 120) << (line.size() > 120 ? "..." : "") << "\n";

    std::cout << "\nTrainingExample struct:\n";
    std::cout << "  system_prompt      — sets model role/behavior\n";
    std::cout << "  user_message       — the input\n";
    std::cout << "  assistant_response — the expected output\n";
    return 0;
}
