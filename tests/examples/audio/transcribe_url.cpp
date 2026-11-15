#define LLM_AUDIO_IMPLEMENTATION
#include "llm_audio.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
// This example downloads audio bytes from a URL via curl, then transcribes.
// In practice, pipe audio from any source into transcribe_bytes().
int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    // Simulate downloaded audio bytes (in real code, download via curl/wget)
    // Here we just show how to use the bytes API with a placeholder.
    std::cout << "transcribe_url: demonstrates transcribe_bytes() API\n\n";
    std::cout << "Usage pattern:\n";
    std::cout << "  1. Download audio from URL into std::vector<uint8_t>\n";
    std::cout << "  2. Call llm::transcribe_bytes(bytes, \"audio.mp3\", cfg)\n\n";

    // Example: create dummy bytes (not valid audio — just shows the call)
    std::vector<uint8_t> fake_audio(1024, 0);

    llm::TranscribeConfig cfg;
    cfg.api_key         = key;
    cfg.response_format = "text";

    // This will fail with invalid audio, but demonstrates the API
    std::cout << "Calling transcribe_bytes() with " << fake_audio.size() << " bytes...\n";
    std::cout << "(Will fail because bytes are not valid audio — for demo only)\n";
    try {
        auto r = llm::transcribe_bytes(fake_audio, "audio.mp3", cfg);
        std::cout << "Result: " << r.text << "\n";
    } catch (const std::exception& e) {
        std::cout << "API error (expected with fake audio): " << e.what() << "\n";
    }
    return 0;
}
