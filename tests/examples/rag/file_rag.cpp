#define LLM_RAG_IMPLEMENTATION
#include "llm_rag.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (!key || !*key) { std::cerr << "Set OPENAI_API_KEY\n"; return 1; }

    // Use provided file or create a sample
    std::string filepath = (argc > 1) ? argv[1] : "sample_knowledge.txt";

    if (argc <= 1) {
        std::ofstream f("sample_knowledge.txt");
        f << "CMake is a cross-platform build system generator.\n"
             "It generates native build files (Makefiles, MSVC projects, Ninja) from a CMakeLists.txt.\n"
             "Modern CMake uses targets and properties rather than global variables.\n\n"
             "The find_package command locates external libraries.\n"
             "CURL can be found with: find_package(CURL REQUIRED)\n"
             "Then link with: target_link_libraries(myapp CURL::libcurl)\n\n"
             "Generator expressions like $<TARGET_FILE:target> are evaluated at build time.\n"
             "Use cmake --build build instead of make directly for portability.\n";
        std::cout << "Created sample_knowledge.txt\n";
    }

    llm::RagConfig cfg;
    cfg.embed_api_key = key;
    cfg.llm_api_key   = key;
    cfg.chunk_size    = 400;
    cfg.chunk_overlap = 80;
    cfg.top_k         = 3;
    cfg.index_path    = "file_rag.index";

    llm::RagPipeline rag(cfg);

    // Try loading saved index first
    rag.load_index();
    if (rag.chunk_count() == 0) {
        std::cout << "Ingesting " << filepath << "...\n";
        rag.ingest_file(filepath);
        rag.save_index();
        std::cout << "Indexed " << rag.chunk_count() << " chunks, saved to " << cfg.index_path << "\n";
    } else {
        std::cout << "Loaded " << rag.chunk_count() << " chunks from " << cfg.index_path << "\n";
    }

    // Query loop
    std::string question;
    std::cout << "\nRAG query loop (type 'quit' to exit)\n";
    while (true) {
        std::cout << "\nQ: ";
        if (!std::getline(std::cin, question) || question == "quit") break;
        if (question.empty()) continue;

        auto result = rag.query(question);
        std::cout << "A: " << result.answer << "\n";
        std::cout << "  [Retrieved " << result.retrieved_chunks.size() << " chunks]\n";
    }
    return 0;
}
