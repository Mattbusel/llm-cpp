#define LLM_CHAT_IMPLEMENTATION
#include "llm_chat.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

static const char* HISTORY_FILE = "chat_history.jsonl";

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::ChatConfig cfg;
    cfg.api_key       = key;
    cfg.model         = "gpt-4o-mini";
    cfg.system_prompt = "You are a helpful assistant who remembers our conversation.";
    cfg.max_tokens    = 256;

    llm::Conversation conv(cfg);

    // Try loading previous session
    std::ifstream f(HISTORY_FILE);
    if (f) {
        std::string jsonl((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
        conv.deserialize(jsonl);
        std::cout << "Loaded " << conv.message_count()
                  << " messages from " << HISTORY_FILE << "\n\n";
    } else {
        std::cout << "Starting fresh session (no history file found)\n\n";
    }

    // Send a message
    std::string reply = conv.chat("What did we talk about previously, if anything?");
    std::cout << "AI: " << reply << "\n\n";

    reply = conv.chat("Good. Remember that my favorite color is blue.");
    std::cout << "AI: " << reply << "\n\n";

    // Save for next session
    std::ofstream out(HISTORY_FILE);
    out << conv.serialize();
    std::cout << "Session saved to " << HISTORY_FILE
              << " (" << conv.message_count() << " messages)\n";
    return 0;
}
