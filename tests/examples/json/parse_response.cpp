// parse_response.cpp: parse a typical OpenAI API response
#define LLM_JSON_IMPLEMENTATION
#include "llm_json.hpp"
#include <cstdio>

int main() {
    const char* response = R"({
  "id": "chatcmpl-abc123",
  "object": "chat.completion",
  "model": "gpt-4o-mini",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "The capital of France is Paris."
      },
      "finish_reason": "stop"
    }
  ],
  "usage": {
    "prompt_tokens": 15,
    "completion_tokens": 8,
    "total_tokens": 23
  }
})";

    auto doc = llm::json::parse(response);

    auto& choices = doc["choices"];
    auto& msg     = choices[0]["message"];

    std::printf("ID:      %s\n", doc["id"].as_string().c_str());
    std::printf("Model:   %s\n", doc["model"].as_string().c_str());
    std::printf("Content: %s\n", msg["content"].as_string().c_str());
    std::printf("Tokens:  %lld\n", doc["usage"]["total_tokens"].as_int());
    return 0;
}
