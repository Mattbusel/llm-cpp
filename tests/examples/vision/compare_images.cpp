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

    // Two public images via URL
    llm::VisionImage img1, img2;
    img1.url = "https://upload.wikimedia.org/wikipedia/commons/thumb/4/47/"
               "PNG_transparency_demonstration_1.png/240px-PNG_transparency_demonstration_1.png";
    img2.url = "https://upload.wikimedia.org/wikipedia/commons/thumb/a/a7/"
               "Camponotus_flavomarginatus_ant.jpg/320px-Camponotus_flavomarginatus_ant.jpg";

    std::cout << "Comparing two images...\n";
    std::string answer = llm::vision(
        "Describe each image briefly and compare them. What are the main differences?",
        {img1, img2}, cfg);

    std::cout << "\nComparison:\n" << answer << "\n";
    return 0;
}
