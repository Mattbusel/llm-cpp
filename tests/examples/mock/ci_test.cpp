#define LLM_MOCK_IMPLEMENTATION
#include "llm_mock.hpp"
#include <iostream>
#include <cassert>

// Simulates a CI test using install_mock / uninstall_mock

std::string my_app_logic(const std::string& user_input) {
    return llm::call_mock(user_input);
}

int main() {
    llm::ScriptedResponses scripted;
    scripted.responses = {"Paris", "4", "Albert Einstein"};

    llm::MockConfig cfg;
    cfg.strategy = scripted;

    llm::install_mock(cfg);

    auto r1 = my_app_logic("Capital of France?");
    auto r2 = my_app_logic("2 + 2 = ?");
    auto r3 = my_app_logic("Who invented relativity?");

    assert(r1 == "Paris");
    assert(r2 == "4");
    assert(r3 == "Albert Einstein");

    llm::uninstall_mock();

    std::cout << "All CI assertions passed.\n";
    std::cout << "Responses: " << r1 << ", " << r2 << ", " << r3 << "\n";
}
