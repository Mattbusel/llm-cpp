#define LLM_AGENT_IMPLEMENTATION
#include "llm_agent.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }

    std::vector<llm::Tool> tools = {
        {
            "add", "Add two numbers", {"a", "b"}, {"First number", "Second number"},
            [](std::map<std::string,std::string> args) -> std::string {
                double a = std::stod(args["a"]), b = std::stod(args["b"]);
                return std::to_string(a + b);
            }
        },
        {
            "multiply", "Multiply two numbers", {"a", "b"}, {"First number", "Second number"},
            [](std::map<std::string,std::string> args) -> std::string {
                double a = std::stod(args["a"]), b = std::stod(args["b"]);
                return std::to_string(a * b);
            }
        },
        {
            "divide", "Divide a by b", {"a", "b"}, {"Numerator", "Denominator"},
            [](std::map<std::string,std::string> args) -> std::string {
                double a = std::stod(args["a"]), b = std::stod(args["b"]);
                if (b == 0) throw std::runtime_error("division by zero");
                return std::to_string(a / b);
            }
        }
    };

    llm::AgentConfig cfg;
    cfg.api_key      = key;
    cfg.model        = "gpt-4o-mini";
    cfg.verbose      = true;
    cfg.system_prompt = "You are a calculator assistant. Use the provided tools to compute answers.";

    std::string prompt = "What is (15 + 27) * 3, then divide by 6?";
    std::cout << "Prompt: " << prompt << "\n\n";

    auto result = llm::run_agent(prompt, tools, cfg);

    std::cout << "\n--- Trace ---\n";
    for (size_t i = 0; i < result.steps.size(); ++i) {
        const auto& s = result.steps[i];
        if (s.type == llm::AgentStep::Type::ToolCall) {
            std::cout << "Step " << (i+1) << ": tool=" << s.tool_name << "(";
            bool first = true;
            for (const auto& kv : s.tool_args) {
                if (!first) std::cout << ", ";
                std::cout << kv.first << "=" << kv.second;
                first = false;
            }
            std::cout << ") -> " << s.tool_result << "\n";
        } else {
            std::cout << "Step " << (i+1) << ": [final answer]\n";
        }
    }

    std::cout << "\nAnswer: " << result.answer << "\n";
    std::cout << "Iterations: " << result.iterations_used << "\n";
    return 0;
}
