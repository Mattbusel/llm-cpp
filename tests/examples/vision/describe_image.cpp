#define LLM_VISION_IMPLEMENTATION
#include "llm_vision.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::VisionConfig cfg;
    cfg.api_key   = key;
    cfg.model     = "gpt-4o";
    cfg.provider  = llm::VisionProvider::OpenAI;
    cfg.max_tokens = 512;

    if (argc > 1) {
        // Load from file path provided as argument
        llm::VisionImage img = llm::load_image(argv[1], "image/jpeg");
        std::string answer = llm::vision("Describe this image in one sentence.", {img}, cfg);
        std::cout << "Description: " << answer << "\n";
    } else {
        // Use a public URL instead
        llm::VisionImage img;
        img.url = "https://upload.wikimedia.org/wikipedia/commons/thumb/4/47/PNG_transparency_demonstration_1.png/240px-PNG_transparency_demonstration_1.png";
        std::string answer = llm::vision("What do you see in this image?", {img}, cfg);
        std::cout << "Description: " << answer << "\n";
    }
    return 0;
}
