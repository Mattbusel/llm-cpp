#define LLM_EMBED_IMPLEMENTATION
#include "llm_embed.hpp"
#include <cstdlib>
#include <iostream>
int main() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !key[0]) { std::cerr << "OPENAI_API_KEY not set\n"; return 1; }
    llm::EmbedConfig cfg;
    cfg.api_key = key;
    cfg.model   = "text-embedding-3-small";
    cfg.dimensions = 256; // smaller for demo
    std::string a = "The cat sat on the mat.";
    std::string b = "A feline rested on a rug.";
    std::string c = "The stock market crashed yesterday.";
    auto ea = llm::embed(a, cfg);
    auto eb = llm::embed(b, cfg);
    auto ec = llm::embed(c, cfg);
    std::cout << "Similarity (cat/feline):  " << llm::cosine_similarity(ea, eb) << "\n";
    std::cout << "Similarity (cat/stocks):  " << llm::cosine_similarity(ea, ec) << "\n";
    std::cout << "Euclidean (cat/feline):   " << llm::euclidean_distance(ea, eb) << "\n";
    return 0;
}
