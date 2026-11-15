#define LLM_JSON_IMPLEMENTATION
#include "llm_json.hpp"
#include <iostream>

int main() {
    std::string raw = R"({
        "name": "Alice",
        "age": 30,
        "scores": [9.5, 8.0, 10.0],
        "address": {"city": "Paris", "zip": "75001"},
        "active": true,
        "notes": null
    })";

    auto v = llm::json::parse(raw);
    std::cout << "name:   " << v["name"].as_string() << "\n";
    std::cout << "age:    " << v["age"].as_int() << "\n";
    std::cout << "city:   " << v["address"]["city"].as_string() << "\n";
    std::cout << "score0: " << v["scores"][0].as_float() << "\n";
    std::cout << "active: " << (v["active"].as_bool() ? "true" : "false") << "\n";
    std::cout << "notes null: " << (v["notes"].is_null() ? "yes" : "no") << "\n";

    // Build JSON
    auto obj = llm::json::make_object({
        {"model", "gpt-4o-mini"},
        {"max_tokens", 1024},
        {"stream", false},
    });
    obj["messages"] = llm::json::array();
    obj["messages"].push_back(llm::json::make_object({
        {"role", "user"}, {"content", "Hello!"}
    }));

    std::cout << "\nBuilt JSON (compact):\n" << obj.dump() << "\n";
    std::cout << "\nBuilt JSON (pretty):\n"  << obj.dump(2) << "\n";

    // try_parse
    auto res = llm::json::try_parse("{bad json}");
    std::cout << "\ntry_parse bad: ok=" << (res.ok ? "true" : "false")
              << " error=" << res.error << "\n";

    return 0;
}
