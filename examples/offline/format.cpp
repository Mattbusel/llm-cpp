#define LLM_FORMAT_IMPLEMENTATION
#include "llm_format.hpp"
#include <cstdio>

int main() {
    llm::Schema schema;
    schema.name = "Ticket";
    schema.fields = {{"title", "string"},
                     {"priority", "number"},
                     {"tags", "array"}};

    // Stand-in for a model: the first reply is wrapped in markdown and
    // has the wrong type; the re-prompted reply is correct.
    int turn = 0;
    auto model = [&](const std::string&) -> std::string {
        if (++turn == 1)
            return "```json\n{\"title\": \"Login fails\", "
                   "\"priority\": \"high\"}\n```";
        return R"({"title": "Login fails", "priority": 1,
                   "tags": ["auth"]})";
    };

    auto r = llm::enforce_schema("File a ticket: users cannot log in",
                                 schema, model);
    std::printf("valid: %s after %d attempt(s)\n",
                r.valid ? "yes" : "no", r.attempts_used);
    std::printf("%s\n", llm::to_json(r.value, true).c_str());

    auto check = llm::validate(llm::parse_json(R"({"title": 7})"), schema);
    for (const auto& e : check.errors)
        std::printf("error: %s\n", e.c_str());
}
