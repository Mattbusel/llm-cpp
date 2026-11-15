// Links every available implementation (tests/headers/*.cpp) into one binary
// and includes the umbrella header, so the 26 libraries can live together.
#include <llm/llm.hpp>

#include <cstdio>
#include <cstring>

int main() {
    std::printf("llm-cpp %s (%d.%d.%d)\n", LLM_CPP_VERSION,
                LLM_CPP_VERSION_MAJOR, LLM_CPP_VERSION_MINOR, LLM_CPP_VERSION_PATCH);
    return std::strcmp(LLM_CPP_VERSION, LLM_CPP_EXPECTED_VERSION) == 0 ? 0 : 1;
}
