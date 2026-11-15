#define LLM_BATCH_IMPLEMENTATION
#include "llm_batch.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

static void create_sample_prompts(const std::string& path) {
    std::ofstream f(path);
    f << "{\"id\":\"p1\",\"prompt\":\"What is the capital of Japan? One word.\"}\n"
      << "{\"id\":\"p2\",\"prompt\":\"What is 7 times 8? One number.\"}\n"
      << "{\"id\":\"p3\",\"prompt\":\"Name the largest planet. One word.\"}\n"
      << "{\"id\":\"p4\",\"prompt\":\"What element is H2O? One word.\"}\n"
      << "{\"id\":\"p5\",\"prompt\":\"Who wrote Hamlet? Last name only.\"}\n";
    std::cout << "Created " << path << "\n";
}

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    const std::string input_path  = "prompts.jsonl";
    const std::string output_path = "results.jsonl";

    create_sample_prompts(input_path);

    llm::BatchConfig cfg;
    cfg.api_key        = key;
    cfg.model          = "gpt-4o-mini";
    cfg.max_tokens     = 30;
    cfg.num_threads    = 3;
    cfg.rate_limit_rps = 5.0;
    cfg.verbose        = true;

    std::cout << "\nProcessing " << input_path << " -> " << output_path << "\n\n";

    size_t n = llm::process_file(input_path, output_path, cfg);

    std::cout << "\nSuccessfully processed: " << n << " items\n";
    std::cout << "Results written to: " << output_path << "\n";

    // Print results file
    std::ifstream rf(output_path);
    std::string line;
    std::cout << "\nOutput JSONL (first 80 chars per line):\n";
    while (std::getline(rf, line))
        std::cout << "  " << line.substr(0, 80) << "\n";

    return 0;
}
