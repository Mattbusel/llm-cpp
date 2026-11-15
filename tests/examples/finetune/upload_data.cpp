#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    // Write training data to a temp file and upload it
    std::vector<llm::TrainingExample> examples;
    for (int i = 0; i < 10; ++i) {
        examples.push_back({
            "You answer math questions.",
            "What is " + std::to_string(i) + " * 2?",
            std::to_string(i * 2) + ".",
        });
    }

    std::printf("Writing training.jsonl...\n");
    llm::write_training_jsonl(examples, "training.jsonl");

    std::printf("Uploading training.jsonl...\n");
    auto result = llm::upload_training_file("training.jsonl", key);
    std::printf("File ID:  %s\n", result.file_id.c_str());
    std::printf("Filename: %s\n", result.filename.c_str());
    std::printf("Bytes:    %zu\n", result.bytes);
    std::printf("Status:   %s\n", result.status.c_str());
    return 0;
}
