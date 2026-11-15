#define LLM_STREAM_IMPLEMENTATION
#include "llm_stream.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main() {
    llm::Config cfg;

    const char* anthropic_key = std::getenv("ANTHROPIC_API_KEY");
    const char* openai_key    = std::getenv("OPENAI_API_KEY");

    if (anthropic_key) {
        cfg.api_key = anthropic_key;
        cfg.model   = "claude-3-5-haiku-20241022";
        std::cout << "Using Anthropic (claude-3-5-haiku-20241022)\n";
    } else if (openai_key) {
        cfg.api_key = openai_key;
        cfg.model   = "gpt-4o-mini";
        std::cout << "Using OpenAI (gpt-4o-mini)\n";
    } else {
        std::cerr << "Error: Set ANTHROPIC_API_KEY or OPENAI_API_KEY\n";
        return 1;
    }

    cfg.system_prompt = "You are a helpful assistant.";
    cfg.max_tokens    = 1024;

    struct Turn { std::string role; std::string content; };
    std::vector<Turn> history;

    std::cout << "Chat loop started. Type 'exit' to quit.\n\n";

    while (true) {
        std::cout << "You: ";
        std::string user_input;
        if (!std::getline(std::cin, user_input)) break;

        if (user_input == "exit" || user_input == "quit") {
            std::cout << "Goodbye.\n";
            break;
        }
        if (user_input.empty()) continue;

        // Build prompt from full history.
        // Note: llm::stream() takes a single string prompt. We serialize history
        // here so the model sees prior context. For production use, consider a
        // library that exposes a messages[] API directly.
        std::string prompt;
        for (const auto& turn : history) {
            prompt += (turn.role == "user" ? "Human: " : "Assistant: ");
            prompt += turn.content + "\n";
        }
        prompt += "Human: " + user_input + "\n";

        history.push_back({"user", user_input});

        std::string assistant_reply;
        std::cout << "\nAssistant: ";

        llm::stream(
            prompt, cfg,
            [&](std::string_view token) {
                std::cout << token << std::flush;
                assistant_reply += token;
            },
            [](const llm::StreamStats& s) {
                std::cout << "\n[" << s.token_count << " tokens, "
                          << s.tokens_per_sec << " tok/s]\n\n";
            },
            [](std::string_view err) {
                std::cerr << "\nError: " << err << "\n";
            }
        );

        history.push_back({"assistant", assistant_reply});
    }

    return 0;
}
