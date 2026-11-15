#define LLM_FORMAT_IMPLEMENTATION
#include "llm_format.hpp"
#include <iostream>

int main() {
    llm::Schema schema;
    schema.name = "ContactInfo";
    schema.fields = {
        {"email",   "string", true,  "Email address"},
        {"phone",   "string", false, "Phone number (optional)"},
        {"country", "string", true,  "Country code"},
    };

    // --- Case 1: Valid JSON ---
    {
        const std::string json = R"({"email": "user@example.com", "phone": "+1-555-0100", "country": "US"})";
        llm::JsonValue value = llm::parse_json(json);
        llm::ValidationResult vr = llm::validate(value, schema);

        std::cout << "=== Case 1: Valid JSON ===\n";
        if (vr.valid) {
            std::cout << "Validation passed.\n";
            std::cout << "  email:   " << value["email"].as_string()   << "\n";
            std::cout << "  phone:   " << value["phone"].as_string()   << "\n";
            std::cout << "  country: " << value["country"].as_string() << "\n";
        } else {
            std::cout << "Validation failed (unexpected):\n";
            for (const auto& e : vr.errors) std::cout << "  - " << e << "\n";
        }
        std::cout << "\n";
    }

    // --- Case 2: Missing required field ---
    {
        const std::string json = R"({"email": "alice@example.com"})";
        llm::JsonValue value = llm::parse_json(json);
        llm::ValidationResult vr = llm::validate(value, schema);

        std::cout << "=== Case 2: Missing required field 'country' ===\n";
        if (vr.valid) {
            std::cout << "Validation passed (unexpected).\n";
        } else {
            std::cout << "Validation failed as expected. Errors:\n";
            for (const auto& e : vr.errors) std::cout << "  - " << e << "\n";
        }
        std::cout << "\n";
    }

    // --- Case 3: to_json round-trip ---
    {
        const std::string original = R"({"country": "DE", "email": "bob@example.com", "score": 42})";
        llm::JsonValue value = llm::parse_json(original);
        std::string serialized = llm::to_json(value, /*pretty=*/true);

        std::cout << "=== Case 3: parse -> to_json round-trip ===\n";
        std::cout << "Serialized (pretty):\n" << serialized << "\n";
    }
}
