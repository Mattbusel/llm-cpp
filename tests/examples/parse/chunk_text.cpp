#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <iostream>

int main() {
    std::string text =
        "The history of computing spans decades of innovation. In the early days, "
        "computers filled entire rooms. Transistors replaced vacuum tubes. "
        "Integrated circuits then packed thousands of transistors onto a single chip. "
        "The microprocessor era began in the 1970s. Personal computers emerged in the 1980s. "
        "The internet transformed everything in the 1990s. "
        "Today, billions of devices are connected worldwide. "
        "Artificial intelligence is now reshaping every industry. "
        "Machine learning models process vast amounts of data. "
        "The future holds quantum computing and neuromorphic chips.";

    llm::ChunkConfig cfg;
    cfg.chunk_size           = 150;
    cfg.overlap              = 30;
    cfg.split_on_sentences   = true;
    cfg.split_on_paragraphs  = true;

    auto chunks = llm::chunk(text, cfg);
    std::cout << "Text length: " << text.size() << " chars\n";
    std::cout << "Chunks: " << chunks.size() << "\n\n";
    for (size_t i = 0; i < chunks.size(); ++i) {
        std::cout << "--- Chunk " << i + 1 << " (" << chunks[i].size() << " chars) ---\n";
        std::cout << chunks[i] << "\n\n";
    }
    return 0;
}
