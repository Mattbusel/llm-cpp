#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <iostream>

int main() {
    std::string html = R"(
<!DOCTYPE html>
<html>
<head><title>Test Page</title></head>
<body>
<h1>Welcome</h1>
<p>This is a <b>test</b> page with <a href="https://example.com">a link</a>.</p>
<script>alert('ignored');</script>
<style>body { color: red; }</style>
<h2>Section 2</h2>
<p>Entities: &amp; &lt; &gt; &quot; &nbsp;</p>
</body></html>)";

    auto result = llm::parse_html(html);
    std::cout << "Title: " << result.title << "\n";
    std::cout << "Text:\n" << result.text << "\n";
    std::cout << "Links: " << result.links.size() << "\n";
    for (const auto& l : result.links) std::cout << "  " << l << "\n";
    std::cout << "Headings:\n";
    for (const auto& h : result.headings) std::cout << "  " << h << "\n";

    std::cout << "\nStats:\n";
    auto stats = llm::analyze(result.text);
    std::cout << "  words=" << stats.word_count
              << " sentences=" << stats.sentence_count
              << " likely_html=" << (stats.likely_html ? "yes" : "no") << "\n";
    return 0;
}
