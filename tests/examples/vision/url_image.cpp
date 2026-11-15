#define LLM_VISION_IMPLEMENTATION
#include "llm_vision.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::VisionConfig cfg;
    cfg.api_key    = key;
    cfg.model      = "gpt-4o";
    cfg.provider   = llm::VisionProvider::OpenAI;
    cfg.max_tokens = 256;

    // Pass URL directly — no base64 encoding needed
    llm::VisionImage img;
    img.url = "https://upload.wikimedia.org/wikipedia/commons/thumb/4/47/"
              "PNG_transparency_demonstration_1.png/240px-PNG_transparency_demonstration_1.png";

    std::cout << "Using image URL directly (no file loading, no base64):\n";
    std::cout << "URL: " << img.url.substr(0, 60) << "...\n\n";

    std::string answer = llm::vision("What is in this image?", {img}, cfg);
    std::cout << "Response: " << answer << "\n";
    return 0;
}
