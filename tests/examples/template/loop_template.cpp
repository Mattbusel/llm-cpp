#define LLM_TEMPLATE_IMPLEMENTATION
#include "llm_template.hpp"
#include <iostream>

int main() {
    llm::Template tmpl(
        "Analyze the following items:\n"
        "{{#items}}"
        "- {{name}} (category: {{category}}, score: {{score}})\n"
        "{{/items}}"
        "\nProvide a summary."
    );

    llm::TemplateContext ctx;
    ctx.lists["items"] = {
        {{"name","Apple"},   {"category","fruit"},    {"score","9.2"}},
        {{"name","Carrot"},  {"category","vegetable"}, {"score","8.7"}},
        {{"name","Chicken"}, {"category","protein"},   {"score","8.5"}},
    };

    std::cout << tmpl.render(ctx) << "\n";
    return 0;
}
