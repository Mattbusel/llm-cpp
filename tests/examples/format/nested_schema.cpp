#define LLM_FORMAT_IMPLEMENTATION
#include "llm_format.hpp"
#include <iostream>

// Mock LLM that returns a response with an array field
static std::string call_llm(const std::string& prompt) {
    (void)prompt;
    return R"({"name": "Alice", "skills": ["C++", "Rust", "Python"], "years_exp": 8})";
}

int main() {
    llm::Schema schema;
    schema.name = "DeveloperProfile";
    schema.fields = {
        {"name",      "string", true, "Full name of the developer"},
        {"skills",    "array",  true, "List of programming languages or technologies"},
        {"years_exp", "number", true, "Years of professional experience"},
    };

    const std::string text =
        "Alice is a senior engineer with 8 years of experience. "
        "She is proficient in C++, Rust, and Python.";
    const std::string prompt =
        "Extract the developer's profile from this text: \"" + text + "\"";

    llm::FormatResult result = llm::enforce_schema(prompt, schema, call_llm);

    if (result.valid) {
        std::cout << "Developer Profile:\n";
        std::cout << "  name:      " << result.value["name"].as_string() << "\n";
        std::cout << "  years_exp: " << result.value["years_exp"].as_number() << "\n";
        std::cout << "  skills:\n";

        const llm::JsonValue& skills = result.value["skills"];
        for (size_t i = 0; i < skills.size(); ++i) {
            std::cout << "    - " << skills[i].as_string() << "\n";
        }
        std::cout << "\nAttempts used: " << result.attempts_used << "\n";
    } else {
        std::cerr << "Failed to extract valid JSON after " << result.attempts_used << " attempts.\n";
        std::cerr << "Raw response: " << result.raw_response << "\n";
        return 1;
    }
}
