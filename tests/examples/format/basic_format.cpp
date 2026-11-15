#define LLM_FORMAT_IMPLEMENTATION
#include "llm_format.hpp"
#include <iostream>
#include <cstdlib>

// Simulate an LLM call — replace with llm::stream or your actual API client
static std::string call_llm(const std::string& prompt) {
    // In real use: call OpenAI/Anthropic API here and return the text response.
    // For this example we return a hardcoded response to show the parsing pipeline.
    (void)prompt;
    return R"({"name": "John Smith", "age": 34, "city": "Seattle"})";
}

int main() {
    llm::Schema schema;
    schema.name = "PersonInfo";
    schema.fields = {
        {"name", "string",  true, "Full name of the person"},
        {"age",  "number",  true, "Age in years"},
        {"city", "string",  true, "City of residence"},
    };

    const std::string text   = "John Smith is 34 years old and lives in Seattle.";
    const std::string prompt = "Extract the person's information from this text: \"" + text + "\"";

    llm::FormatResult result = llm::enforce_schema(prompt, schema, call_llm);

    if (result.valid) {
        std::cout << "Extracted:\n";
        std::cout << "  name: " << result.value["name"].as_string() << "\n";
        std::cout << "  age:  " << result.value["age"].as_number()  << "\n";
        std::cout << "  city: " << result.value["city"].as_string() << "\n";
        std::cout << "\nAttempts used: " << result.attempts_used << "\n";
    } else {
        std::cerr << "Failed to extract valid JSON after " << result.attempts_used << " attempts.\n";
        std::cerr << "Raw response: " << result.raw_response << "\n";
        return 1;
    }
}
