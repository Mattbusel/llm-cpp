#define LLM_RAG_IMPLEMENTATION
#include "llm_rag.hpp"
#include <iostream>

int main() {
    std::string text =
        "Rust is a systems programming language focused on safety and performance. "
        "It achieves memory safety without a garbage collector using its ownership system. "
        "Rust's borrow checker ensures that references do not outlive the data they point to. "
        "Zero-cost abstractions allow high-level code to compile to efficient machine code. "
        "The language is well-suited for embedded systems, OS development, and web servers.";

    auto chunks = llm::chunk_text(text, 120, 20);

    std::cout << "Chunked into " << chunks.size() << " passages:\n\n";
    for (size_t i = 0; i < chunks.size(); ++i) {
        std::cout << "[" << i << "] (" << chunks[i].size() << " chars)\n"
                  << chunks[i] << "\n\n";
    }
    return 0;
}
