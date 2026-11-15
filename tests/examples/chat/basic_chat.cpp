#define LLM_CHAT_IMPLEMENTATION
#include "llm_chat.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::ChatConfig cfg;
    cfg.api_key      = key;
    cfg.model        = "gpt-4o-mini";
    cfg.system_prompt = "You are a helpful assistant. Be concise.";
    cfg.token_budget  = 8192;

    llm::Conversation conv(cfg);

    std::cout << "Assistant: " << conv.chat("Hello! What is 2 + 2?") << "\n";
    std::cout << "Assistant: " << conv.chat("Now multiply that by 10.") << "\n";

    std::cout << "\nHistory (" << conv.message_count() << " messages):\n";
    for (const auto& m : conv.history())
        std::cout << "  [" << m.role << "]: " << m.content.substr(0, 60) << "\n";

    // Serialize and restore
    std::string snap = conv.serialize();
    llm::Conversation conv2(cfg);
    conv2.deserialize(snap);
    std::cout << "\nRestored " << conv2.message_count() << " messages from serialized state\n";
    return 0;
}
