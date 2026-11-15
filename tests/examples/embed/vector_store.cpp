#define LLM_EMBED_IMPLEMENTATION
#include "llm_embed.hpp"
#include <cstdlib>
#include <iostream>
int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !key[0]) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }
    llm::EmbedConfig cfg{ key, "text-embedding-3-small", 256 };
    llm::VectorStore store(".embed_demo_store");
    std::vector<std::string> docs = {
        "Photosynthesis converts sunlight into chemical energy.",
        "The Eiffel Tower is located in Paris, France.",
        "Machine learning is a subset of artificial intelligence.",
        "Mitochondria are the powerhouse of the cell.",
        "The speed of light is approximately 3e8 m/s.",
    };
    std::cout << "Indexing " << docs.size() << " documents...\n";
    for (size_t i = 0; i < docs.size(); ++i) {
        auto emb = llm::embed(docs[i], cfg);
        store.add("doc" + std::to_string(i), docs[i], emb);
    }
    store.save();
    std::string query = "Biology and cells";
    auto qemb = llm::embed(query, cfg);
    auto results = store.search(qemb, 3);
    std::cout << "\nQuery: \"" << query << "\"\nTop 3 results:\n";
    for (auto& r : results)
        std::cout << "  [" << r.similarity << "] " << r.entry.text << "\n";
    return 0;
}
