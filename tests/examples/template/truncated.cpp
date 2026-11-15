#define LLM_TEMPLATE_IMPLEMENTATION
#include "llm_template.hpp"
#include <iostream>

int main() {
    llm::Template tmpl(
        "System: You are a helpful assistant.\n\n"
        "Context:\n{{context}}\n\n"
        "Question: {{question}}\n\n"
        "Answer concisely."
    );

    llm::TemplateContext ctx;
    ctx.vars["question"] = "What are the key points?";
    // Generate a long context
    std::string long_context;
    for (int i = 0; i < 100; ++i)
        long_context += "Sentence " + std::to_string(i) + " of the long document. This is filler text. ";
    ctx.vars["context"] = long_context;

    std::cout << "Full render: " << tmpl.render(ctx).size() << " chars ("
              << tmpl.render(ctx).size()/4 << " est. tokens)\n";

    std::string truncated = tmpl.render_truncated(ctx, 200);
    std::cout << "Truncated to 200 tokens: " << truncated.size() << " chars ("
              << truncated.size()/4 << " est. tokens)\n\n";
    std::cout << truncated << "\n";

    return 0;
}
