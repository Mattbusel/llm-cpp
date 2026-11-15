#define LLM_PARSE_IMPLEMENTATION
#include "llm_parse.hpp"
#include <cstdio>

int main() {
    std::string text = R"(
Machine learning is a branch of artificial intelligence. It focuses on building systems that learn from data.

Deep learning is a subset of machine learning that uses neural networks. These networks have many layers that transform data.

Natural language processing (NLP) enables computers to understand human language. Applications include translation, summarization, and question answering.

Reinforcement learning trains agents to make decisions through trial and error. The agent receives rewards for good actions and penalties for bad ones.
)";

    llm::ChunkConfig cfg;
    cfg.chunk_size = 200;
    cfg.overlap    = 20;

    auto chunks = llm::chunk(text, cfg);
    std::printf("Split into %zu chunks (max %zu chars):\n\n", chunks.size(), cfg.chunk_size);
    for (size_t i = 0; i < chunks.size(); ++i)
        std::printf("--- Chunk %zu (%zu chars) ---\n%s\n\n",
                    i + 1, chunks[i].size(), chunks[i].c_str());
    return 0;
}
