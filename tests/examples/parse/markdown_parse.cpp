#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <cstdio>

int main() {
    std::string md = R"(
# Introduction

This is **bold** and *italic* text.

```python
def hello():
    print("Hello, world!")
```

Check out [OpenAI](https://openai.com) for more info.

## Section 2

Another paragraph with some content.
)";

    auto result = llm::parse_markdown(md);
    std::printf("Stripped text:\n%s\n", result.plain_text.c_str());
    std::printf("\nCode blocks: %zu\n", result.code_blocks.size());
    for (auto& cb : result.code_blocks)
        std::printf("  %zu chars\n", cb.size());
    return 0;
}
