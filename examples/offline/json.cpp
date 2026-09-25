#define LLM_JSON_IMPLEMENTATION
#include "llm_json.hpp"
#include <cstdio>

int main() {
    namespace json = llm::json;

    auto body = json::object();           // build a request body
    body["model"] = "gpt-4o-mini";
    body["temperature"] = 0.5;
    auto msg = json::object();
    msg["role"] = "user";
    msg["content"] = "Say \"hi\"";
    body["messages"].push_back(msg);
    std::printf("%s\n\n", body.dump_pretty().c_str());

    auto resp = json::parse(R"({"choices":[{"message":{"content":"hi!"}}],
                               "usage":{"total_tokens":17}})");
    auto& text = resp["choices"][0]["message"]["content"];
    std::printf("content: %s\ntokens:  %lld\n", text.as_string().c_str(),
                resp["usage"]["total_tokens"].as_int());

    auto bad = json::try_parse(R"({"choices": [}")");
    std::printf("\nbad input -> ok=%s, %s\n",
                bad.ok ? "true" : "false", bad.error.c_str());
}
