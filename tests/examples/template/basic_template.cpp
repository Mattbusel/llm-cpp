#define LLM_TEMPLATE_IMPLEMENTATION
#include "llm_template.hpp"
#include <iostream>

int main() {
    llm::Template tmpl("Hello, {{name}}! You are a {{role}} assistant. Today is {{date}}.");

    llm::TemplateVars vars;
    vars["name"] = "Claude";
    vars["role"] = "helpful";
    vars["date"] = "2026-03-06";

    std::cout << tmpl.render(vars) << "\n\n";

    // Show missing vars detection
    llm::TemplateContext ctx;
    ctx.vars["name"] = "Claude";
    // missing: role, date
    auto missing = tmpl.missing_vars(ctx);
    std::cout << "Missing vars: ";
    for (const auto& v : missing) std::cout << v << " ";
    std::cout << "\n\n";

    // List all variables
    std::cout << "Variables in template: ";
    for (const auto& v : tmpl.variables()) std::cout << v << " ";
    std::cout << "\n";

    return 0;
}
