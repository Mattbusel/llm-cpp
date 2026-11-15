// pipeline.cpp: transcribe audio -> speak the transcription back in a different voice
#define LLM_AUDIO_IMPLEMENTATION
#include "llm_audio.hpp"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key) { std::puts("Set OPENAI_API_KEY"); return 1; }
    const char* file = (argc > 1) ? argv[1] : "audio.mp3";

    try {
        // Step 1: transcribe
        llm::TranscribeConfig tc;
        tc.api_key = key;
        auto result = llm::transcribe(file, tc);
        std::printf("Transcribed: %s\n", result.text.c_str());

        // Step 2: speak back in a different voice
        llm::TTSConfig tts;
        tts.api_key = key;
        tts.voice   = llm::TTSVoice::Shimmer;
        llm::text_to_speech(result.text, "pipeline_output.mp3", tts);
        std::printf("Re-spoken to: pipeline_output.mp3\n");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what()); return 1;
    }
    return 0;
}
