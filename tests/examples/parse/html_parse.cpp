#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <cstdio>

int main() {
    std::string html = R"(
<html>
<head><title>Test Page</title></head>
<body>
  <h1>Hello World</h1>
  <p>This is a <b>test</b> paragraph with a <a href="https://example.com">link</a>.</p>
  <script>alert('ignored');</script>
  <p>Second paragraph &amp; special &lt;chars&gt;.</p>
</body>
</html>)";

    auto result = llm::parse_html(html);
    std::printf("Text:\n%s\n", result.text.c_str());
    std::printf("\nHeadings: %zu\n", result.headings.size());
    for (auto& h : result.headings) std::printf("  %s\n", h.c_str());
    std::printf("\nLinks: %zu\n", result.links.size());
    for (auto& l : result.links) std::printf("  %s\n", l.c_str());
    return 0;
}
