#define LLM_TEMPLATE_IMPLEMENTATION
#include "llm_template.hpp"
#include <iostream>

int main() {
    llm::TemplateRegistry reg;

    reg.add("greeting", "Hello, {{name}}! Welcome to {{service}}.");
    reg.add("qa",       "Q: {{question}}\nA: {{answer}}");
    reg.add("system",   "You are {{persona}}. {{instructions}}");

    llm::TemplateVars v1; v1["name"]="Alice"; v1["service"]="LLM Suite";
    std::cout << reg.get("greeting").render(v1) << "\n";

    llm::TemplateVars v2; v2["question"]="What is C++?"; v2["answer"]="A fast systems language.";
    std::cout << reg.get("qa").render(v2) << "\n";

    llm::TemplateVars v3; v3["persona"]="an expert engineer"; v3["instructions"]="Be concise and precise.";
    std::cout << reg.get("system").render(v3) << "\n";

    std::cout << "\nRegistry has 'greeting': " << reg.has("greeting") << "\n";
    std::cout << "Registry has 'missing':  " << reg.has("missing")  << "\n";

    return 0;
}
