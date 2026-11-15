#define LLM_AUDIO_IMPLEMENTATION
#include "llm_audio.hpp"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }
    const char* text = (argc > 1) ? argv[1] : "Hello from llm-audio text to speech!";
    const char* out  = (argc > 2) ? argv[2] : "output.mp3";

    llm::TTSConfig cfg;
    cfg.api_key = key;
    cfg.voice   = llm::TTSVoice::Nova;

    try {
        llm::text_to_speech(text, out, cfg);
        std::printf("Saved speech to: %s\n", out);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what()); return 1;
    }
    return 0;
}
