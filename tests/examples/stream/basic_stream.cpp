#define LLM_STREAM_IMPLEMENTATION
#include "llm_stream.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) {
        std::cerr << "Error: OPENAI_API_KEY not set\n";
        return 1;
    }

    llm::Config cfg;
    cfg.api_key = key;
    cfg.model   = "gpt-4o-mini";

    std::cout << "Streaming from OpenAI...\n\n";

    llm::stream_openai(
        "Explain recursion in one paragraph.",
        cfg,
        [](std::string_view token) {
            std::cout << token << std::flush;
        },
        [](const llm::StreamStats& s) {
            std::cout << "\n\n[Done: " << s.token_count << " tokens, "
                      << s.elapsed_ms << " ms, "
                      << s.tokens_per_sec << " tok/s]\n";
        },
        [](std::string_view err) {
            std::cerr << "\nError: " << err << "\n";
        }
    );

    return 0;
}
