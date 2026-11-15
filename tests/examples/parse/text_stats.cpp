#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <cstdio>

int main() {
    std::string texts[] = {
        "Hello world! This is a simple sentence.",
        "<html><body><h1>HTML Content</h1><p>A paragraph.</p></body></html>",
        "```python\nfor i in range(10):\n    print(i)\n```",
        "# Markdown\n\nSome **bold** text and a [link](https://example.com).",
    };

    for (auto& text : texts) {
        auto stats = llm::analyze(text);
        std::printf("Text: %.40s...\n", text.c_str());
        std::printf("  Words: %zu, Sentences: %zu, Chars: %zu\n",
                    stats.word_count, stats.sentence_count, stats.char_count);
        std::printf("  is_html=%d, is_markdown=%d, is_code=%d\n\n",
                    (int)stats.likely_html, (int)stats.likely_markdown, (int)stats.likely_code);
    }
    return 0;
}
