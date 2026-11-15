#define LLM_AUDIO_IMPLEMENTATION
#include "llm_audio.hpp"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }
    const char* file = (argc > 1) ? argv[1] : "audio.mp3";
    llm::TranscribeConfig cfg;
    cfg.api_key = key;
    try {
        auto result = llm::transcribe(file, cfg);
        std::printf("Transcription:\n%s\n", result.text.c_str());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what()); return 1;
    }
    return 0;
}
