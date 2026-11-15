#define LLM_JSON_IMPLEMENTATION
#include "llm_json.hpp"
#include <cstdio>

int main() {
    // Build a JSON object
    llm::json::Value obj;
    obj["name"]    = "Alice";
    obj["age"]     = 30;
    obj["active"]  = true;
    obj["score"]   = 9.5;

    llm::json::Value tags;
    tags.push_back("engineer");
    tags.push_back("researcher");
    obj["tags"] = tags;

    std::printf("Built JSON:\n%s\n\n", llm::json::dump(obj, 2).c_str());

    // Parse it back
    std::string json_str = llm::json::dump(obj);
    auto parsed = llm::json::parse(json_str);
    std::printf("Name: %s\n", parsed["name"].as_string().c_str());
    std::printf("Age:  %lld\n", parsed["age"].as_int());
    std::printf("Tags: %zu items\n", parsed["tags"].size());
    return 0;
}
