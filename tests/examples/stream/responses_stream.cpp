// Stream from OpenAI's Responses API (POST /v1/responses) and print the billed usage.
//   OPENAI_API_KEY=... ./responses_stream
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
    cfg.api_key       = key;
    cfg.model         = "gpt-4.1-mini";
    cfg.max_tokens    = 300;
    cfg.system_prompt = "Answer in plain prose, no lists.";

    int rc = 0;
    llm::stream_openai_responses(
        "Explain recursion in one paragraph.",
        cfg,
        [](std::string_view token) { std::cout << token << std::flush; },
        [](const llm::StreamStats& s) {
            std::cout << "\n\n[" << s.response_id << ": " << s.input_tokens << " input tokens, "
                      << s.output_tokens << " output tokens, " << s.elapsed_ms << " ms]\n";
        },
        [&rc](std::string_view err) {
            std::cerr << "\nError: " << err << "\n";
            rc = 1;
        });
    return rc;
}
