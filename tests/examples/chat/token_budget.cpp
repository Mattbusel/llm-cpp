#define LLM_CHAT_IMPLEMENTATION
#include "llm_chat.hpp"
#include <iostream>

// No API call needed — just shows truncation behavior
int main() {
    llm::ChatConfig cfg;
    cfg.api_key       = "dummy"; // not used in this demo
    cfg.system_prompt = "You are a helpful assistant.";
    cfg.token_budget  = 50;  // very tight budget

    llm::Conversation conv(cfg);

    // Add messages manually by round-tripping serialize/deserialize trick:
    // We build a conversation by constructing it from a JSONL snapshot.
    std::string snapshot =
        "{\"role\":\"system\",\"content\":\"You are a helpful assistant.\"}\n"
        "{\"role\":\"user\",\"content\":\"Tell me about photosynthesis.\"}\n"
        "{\"role\":\"assistant\",\"content\":\"Photosynthesis is the process by which plants convert sunlight into energy.\"}\n"
        "{\"role\":\"user\",\"content\":\"What is the role of chlorophyll?\"}\n"
        "{\"role\":\"assistant\",\"content\":\"Chlorophyll is the green pigment that absorbs light energy for photosynthesis.\"}\n"
        "{\"role\":\"user\",\"content\":\"How do plants store energy?\"}\n"
        "{\"role\":\"assistant\",\"content\":\"Plants store energy as glucose and starch in their cells.\"}\n"
        "{\"role\":\"user\",\"content\":\"What about ATP?\"}\n"
        "{\"role\":\"assistant\",\"content\":\"ATP is the universal energy currency; plants produce it during light reactions.\"}\n";

    conv.deserialize(snapshot);

    std::cout << "BEFORE truncation:\n";
    std::cout << "  Messages: " << conv.message_count() << "\n";
    for (const auto& m : conv.history())
        std::cout << "  [" << m.role << "] " << m.content.substr(0, 50) << "\n";

    // Trigger truncation by adding a new user message
    // (trim_to_budget runs internally before each API call)
    // We can simulate by serializing then checking message count
    // after a manual trim by creating a new conv with same budget
    llm::ChatConfig cfg2 = cfg;
    cfg2.token_budget = 50;
    llm::Conversation conv2(cfg2);
    conv2.deserialize(snapshot);

    // Manually trigger trim: add a dummy message then check
    // Actually, trim happens in chat() — show it visually
    std::cout << "\nBudget: " << cfg.token_budget << " tokens (~"
              << cfg.token_budget * 4 << " chars)\n";
    std::cout << "Oldest non-system messages will be dropped to fit budget.\n\n";

    std::cout << "AFTER truncation (messages that would be sent to API):\n";
    // Simulate: remove from front until under budget
    auto msgs = conv.history();
    auto est = [](const std::vector<llm::ChatMessage>& v) {
        size_t c = 0; for (auto& m : v) c += m.content.size(); return (c + 3) / 4;
    };
    size_t budget = 50;
    size_t sys = (!msgs.empty() && msgs[0].role == "system") ? 1 : 0;
    while (est(msgs) > budget && msgs.size() > sys + 1)
        msgs.erase(msgs.begin() + static_cast<long>(sys));

    std::cout << "  Messages kept: " << msgs.size() << "\n";
    for (const auto& m : msgs)
        std::cout << "  [" << m.role << "] " << m.content.substr(0, 50) << "\n";

    return 0;
}
