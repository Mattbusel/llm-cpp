#define LLM_RAG_IMPLEMENTATION
#include "llm_rag.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !key[0]) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }

    llm::RagConfig cfg;
    cfg.embed_api_key = key;
    cfg.llm_api_key   = key;
    cfg.chunk_size    = 200;
    cfg.chunk_overlap = 40;
    cfg.top_k         = 3;
    cfg.index_path    = ".rag_demo_index";

    llm::RagPipeline rag(cfg);

    // Ingest knowledge base
    rag.ingest(
        "The mitochondria is the powerhouse of the cell. "
        "It produces ATP through cellular respiration. "
        "Mitochondria have their own DNA, separate from the cell nucleus. "
        "They are thought to have originated from ancient bacteria (endosymbiosis).",
        "biology"
    );
    rag.ingest(
        "The Eiffel Tower was built by Gustave Eiffel for the 1889 World's Fair in Paris. "
        "It stands 330 meters tall and was the world's tallest structure for 41 years. "
        "It was originally intended to be dismantled after 20 years.",
        "history"
    );

    rag.save_index();
    std::cout << "Indexed " << rag.chunk_count() << " chunks.\n\n";

    // Query
    std::string question = "What produces ATP in cells?";
    auto result = rag.query(question);

    std::cout << "Q: " << question << "\n";
    std::cout << "A: " << result.answer << "\n\n";
    std::cout << "Retrieved " << result.retrieved_chunks.size() << " chunks.\n";
    return 0;
}
