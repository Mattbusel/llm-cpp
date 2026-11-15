#define LLM_FINETUNE_IMPLEMENTATION
#include "llm_finetune.hpp"
#include <cstdio>
#include <cstdlib>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }

    // Build training data
    std::vector<llm::TrainingExample> examples = {
        {"You are a helpful assistant.", "What is 2+2?",            "2+2 equals 4."},
        {"You are a helpful assistant.", "What color is the sky?",  "The sky is blue."},
        {"You are a helpful assistant.", "What is the capital of France?", "Paris is the capital of France."},
        {"You are a helpful assistant.", "How many days in a week?", "There are 7 days in a week."},
        {"You are a helpful assistant.", "What is water made of?",  "Water is made of hydrogen and oxygen (H2O)."},
    };

    // Upload training data
    std::printf("Uploading training data...\n");
    auto upload = llm::upload_examples(examples, key);
    std::printf("File ID: %s  (%zu bytes)\n", upload.file_id.c_str(), upload.bytes);

    if (upload.file_id.empty()) { std::puts("Upload failed"); return 1; }

    // Create fine-tune job
    llm::FinetuneConfig cfg;
    cfg.api_key    = key;
    cfg.base_model = "gpt-4o-mini-2024-07-18";
    cfg.n_epochs   = 3;
    cfg.suffix     = "demo";

    std::printf("Creating fine-tune job...\n");
    auto job = llm::create_job(upload.file_id, cfg);
    std::printf("Job ID: %s  Status: %s\n", job.id.c_str(), job.status.c_str());

    // Poll until done
    std::printf("Polling (this takes minutes to hours)...\n");
    auto final = llm::wait_for_completion(job.id, key, 30, [](const llm::FinetuneJob& j) {
        std::printf("  Status: %s\n", j.status.c_str());
    });

    if (final.status == "succeeded")
        std::printf("Done! Model: %s\n", final.fine_tuned_model.c_str());
    else
        std::printf("Job ended with status: %s\n  Error: %s\n",
                    final.status.c_str(), final.error.c_str());
    return 0;
}
