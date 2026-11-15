#define LLM_COMPRESS_IMPLEMENTATION
#include "llm_compress.hpp"
#include <iostream>

int main() {
    // Build a conversation that exceeds the token budget
    std::vector<llm::CompressMessage> messages = {
        {"system",    "You are a helpful assistant."},
        {"user",      "Hello! Can you explain how neural networks work?"},
        {"assistant", "Neural networks are computational models inspired by the human brain. "
                      "They consist of layers of interconnected nodes (neurons) that process "
                      "information using connectionist approaches."},
        {"user",      "What about backpropagation?"},
        {"assistant", "Backpropagation is the algorithm used to train neural networks. "
                      "It calculates the gradient of the loss function with respect to "
                      "each weight by the chain rule."},
        {"user",      "Can you give me a simple example?"},
        {"assistant", "Sure! Consider a network with one hidden layer. The forward pass "
                      "computes predictions, then we measure the error. Backprop sends "
                      "the error backwards, adjusting weights to minimize loss."},
        {"user",      "What is the learning rate?"},
        {"assistant", "The learning rate is a hyperparameter that controls how much to "
                      "adjust weights during training. Too high and training is unstable; "
                      "too low and training is very slow."},
    };

    std::cout << "Before compression:\n";
    std::cout << "  Messages: " << messages.size() << "\n";
    std::cout << "  Tokens (est): " << llm::estimate_tokens(messages) << "\n\n";

    llm::CompressConfig cfg;
    cfg.strategy     = llm::TruncateHead{};
    cfg.token_budget = 100;  // tight budget

    auto result = llm::compress_messages(messages, cfg);

    std::cout << "After compression (TruncateHead, budget=100 tokens):\n";
    std::cout << "  Messages: " << result.messages.size()
              << " (" << result.messages_removed << " removed)\n";
    std::cout << "  Tokens before: " << result.tokens_before << "\n";
    std::cout << "  Tokens after:  " << result.tokens_after << "\n\n";

    for (auto& m : result.messages)
        std::cout << "  [" << m.role << "] " << m.content.substr(0, 60) << "...\n";
}
