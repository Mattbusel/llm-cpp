#define LLM_AGENT_IMPLEMENTATION
#include "llm_agent.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }

    // Mock search results
    std::map<std::string,std::string> search_db = {
        {"llm streaming",  "LLM streaming uses SSE (Server-Sent Events) to deliver tokens as they are generated."},
        {"token cost",     "GPT-4o costs $2.50 per million input tokens and $10 per million output tokens."},
        {"vector search",  "Vector search uses embedding similarity (cosine or dot product) to find relevant documents."},
        {"circuit breaker","A circuit breaker pattern stops calling a failing service after N failures and retries after a timeout."},
    };

    std::vector<llm::Tool> tools = {
        {
            "search", "Search for information on a topic",
            {"query"}, {"The search query"},
            [&](std::map<std::string,std::string> args) -> std::string {
                const std::string& q = args["query"];
                for (const auto& kv : search_db)
                    if (q.find(kv.first) != std::string::npos || kv.first.find(q) != std::string::npos)
                        return kv.second;
                return "No results found for: " + q;
            }
        },
        {
            "summarize", "Summarize a piece of text into one sentence",
            {"text"}, {"The text to summarize"},
            [](std::map<std::string,std::string> args) -> std::string {
                auto& t = args["text"];
                return t.size() > 80 ? t.substr(0,80) + "..." : t;
            }
        }
    };

    llm::AgentConfig cfg;
    cfg.api_key   = key;
    cfg.model     = "gpt-4o-mini";
    cfg.verbose   = true;
    cfg.max_iterations = 6;

    std::string prompt = "Search for information about LLM streaming and circuit breaker patterns, then give me a combined summary.";
    std::cout << "Prompt: " << prompt << "\n\n";

    auto result = llm::run_agent(prompt, tools, cfg);

    std::cout << "\n--- Steps ---\n";
    for (size_t i = 0; i < result.steps.size(); ++i) {
        const auto& s = result.steps[i];
        if (s.type == llm::AgentStep::Type::ToolCall)
            std::cout << "  [tool] " << s.tool_name << "(" << s.tool_args.begin()->second << ") -> " << s.tool_result << "\n";
        else
            std::cout << "  [done]\n";
    }
    std::cout << "\nFinal answer:\n" << result.answer << "\n";
    return 0;
}
