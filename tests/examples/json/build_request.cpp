// build_request.cpp: construct an OpenAI chat completion request body
#define LLM_JSON_IMPLEMENTATION
#include "llm_json.hpp"
#include <cstdio>

int main() {
    llm::json::Value msg1, msg2;
    msg1["role"]    = "system";
    msg1["content"] = "You are a helpful assistant.";
    msg2["role"]    = "user";
    msg2["content"] = "What is 2 + 2?";

    llm::json::Value messages;
    messages.push_back(msg1);
    messages.push_back(msg2);

    llm::json::Value req;
    req["model"]       = "gpt-4o-mini";
    req["messages"]    = messages;
    req["temperature"] = 0.7;
    req["max_tokens"]  = 100;

    std::printf("%s\n", llm::json::dump(req, 2).c_str());
    return 0;
}
