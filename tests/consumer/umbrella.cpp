#include <llm/llm.hpp>
#include <cstdio>

int main() {
    std::printf("llm-cpp %s\n", LLM_CPP_VERSION);
    return LLM_CPP_VERSION_MAJOR == 1 ? 0 : 1;
}
