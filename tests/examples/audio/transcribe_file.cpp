#define LLM_AUDIO_IMPLEMENTATION
#include "llm_audio.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }
    if (argc < 2) { std::cerr << "Usage: transcribe_file <audio.mp3>\n"; return 1; }

    llm::TranscribeConfig cfg;
    cfg.api_key         = key;
    cfg.model           = "whisper-1";
    cfg.response_format = "text";

    auto result = llm::transcribe(argv[1], cfg);
    std::cout << "Transcript:\n" << result.text << "\n";
    return 0;
}
