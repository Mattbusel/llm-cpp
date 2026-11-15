#define LLM_RAG_IMPLEMENTATION
#include "llm_rag.hpp"
#include <iostream>
int main() {
    std::string text =
        "The water cycle describes how water evaporates from the surface of the earth, "
        "rises into the atmosphere, cools and condenses into rain or snow in clouds, "
        "and falls again to the surface as precipitation. "
        "The water falling on land collects in rivers and lakes, soil, and porous layers of rock, "
        "and much of it flows back into the oceans, where it will once more evaporate. "
        "The cycling of water in and out of the atmosphere is a significant aspect of the weather patterns on Earth. "
        "Photosynthesis and transpiration contribute water vapor to the atmosphere. "
        "The sun drives the water cycle by evaporating water from oceans and lakes. "
        "Condensation forms clouds, and precipitation returns water to the surface.";
    auto chunks = llm::chunk_text(text, 150, 30);
    std::cout << "Text length: " << text.size() << " chars\n"
              << "Chunks:      " << chunks.size() << "\n\n";
    for (size_t i = 0; i < chunks.size(); ++i)
        std::cout << "=== Chunk " << i << " (" << chunks[i].size() << " chars) ===\n"
                  << chunks[i] << "\n\n";
    return 0;
}
