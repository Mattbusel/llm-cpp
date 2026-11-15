#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <iostream>

int main() {
    std::string md = R"(# Getting Started

This is **bold** and *italic* text.

## Code Example

Here is some `inline code` in a sentence.

```python
def hello(name):
    return f"Hello, {name}!"
```

## Links and Lists

- Visit [OpenAI](https://openai.com) for more info
- Check out [GitHub](https://github.com) for code

> This is a blockquote that should be stripped.

The answer is **42** according to _The Hitchhiker's Guide_.
)";

    std::cout << "=== Input Markdown ===\n" << md << "\n";

    auto result = llm::parse_markdown(md);

    std::cout << "=== Plain Text ===\n" << result.plain_text << "\n";

    std::cout << "=== Headings ===\n";
    for (const auto& h : result.headings)
        std::cout << "  " << h << "\n";

    std::cout << "\n=== Code Blocks (" << result.code_blocks.size() << ") ===\n";
    for (const auto& cb : result.code_blocks)
        std::cout << cb;

    std::cout << "\n=== Links ===\n";
    for (const auto& l : result.links)
        std::cout << "  " << l << "\n";

    return 0;
}
