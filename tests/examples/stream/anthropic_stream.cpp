// Stream from Anthropic's Messages API.
//   ANTHROPIC_API_KEY=... ./anthropic_stream
#define LLM_STREAM_IMPLEMENTATION
#include "llm_stream.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("ANTHROPIC_API_KEY");
    if (!key) {
        std::cerr << "Error: ANTHROPIC_API_KEY not set\n";
        return 1;
    }

    llm::Config cfg;
    cfg.api_key    = key;
    cfg.model      = "claude-haiku-4-5";
    cfg.max_tokens = 300;

    int rc = 0;
    // llm::stream routes "claude-*" models to Anthropic; stream_anthropic is the explicit call.
    llm::stream_anthropic(
        "Explain recursion in one paragraph.",
        cfg,
        [](std::string_view token) { std::cout << token << std::flush; },
        [](const llm::StreamStats& s) {
            std::cout << "\n\n[" << s.token_count << " deltas, " << s.elapsed_ms << " ms]\n";
        },
        [&rc](std::string_view err) {
            std::cerr << "\nError: " << err << "\n";
            rc = 1;
        });
    return rc;
}
