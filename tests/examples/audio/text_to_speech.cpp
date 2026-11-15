#define LLM_AUDIO_IMPLEMENTATION
#include "llm_audio.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    llm::TTSConfig cfg;
    cfg.api_key = key;
    cfg.model   = "tts-1";
    cfg.voice   = llm::TTSVoice::Nova;
    cfg.format  = llm::TTSFormat::MP3;
    cfg.speed   = 1.0;

    std::string text = "Hello! This is a test of the OpenAI text-to-speech API from C++.";
    llm::text_to_speech(text, "output.mp3", cfg);
    std::cout << "Saved to output.mp3\n";

    // Also demonstrate bytes API
    auto bytes = llm::text_to_speech_bytes("Short test.", cfg);
    std::cout << "Got " << bytes.size() << " audio bytes\n";
    return 0;
}
