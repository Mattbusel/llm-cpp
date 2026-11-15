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
    cfg.format  = llm::TTSFormat::MP3;

    std::string text = "The quick brown fox jumps over the lazy dog.";

    struct VoiceInfo { llm::TTSVoice voice; const char* name; };
    VoiceInfo voices[] = {
        {llm::TTSVoice::Alloy,   "alloy"},
        {llm::TTSVoice::Echo,    "echo"},
        {llm::TTSVoice::Fable,   "fable"},
        {llm::TTSVoice::Onyx,    "onyx"},
        {llm::TTSVoice::Nova,    "nova"},
        {llm::TTSVoice::Shimmer, "shimmer"},
    };

    for (const auto& v : voices) {
        cfg.voice = v.voice;
        std::string path = std::string("voice_") + v.name + ".mp3";
        auto bytes = llm::text_to_speech_bytes(text, cfg);
        std::cout << v.name << ": " << bytes.size() << " bytes -> " << path << "\n";
    }
    return 0;
}
