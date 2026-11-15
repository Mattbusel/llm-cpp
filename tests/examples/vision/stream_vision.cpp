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
    cfg.max_tokens = 512;

    llm::VisionImage img;
    img.url = "https://upload.wikimedia.org/wikipedia/commons/thumb/4/47/"
              "PNG_transparency_demonstration_1.png/240px-PNG_transparency_demonstration_1.png";

    std::cout << "Streaming vision response:\n";
    std::cout << "---\n";

    std::string full = llm::vision_stream(
        "Describe this image in detail.",
        {img}, cfg,
        [](const std::string& delta) {
            std::cout << delta << std::flush;
        });

    std::cout << "\n---\n";
    std::cout << "Total chars received: " << full.size() << "\n";
    return 0;
}
