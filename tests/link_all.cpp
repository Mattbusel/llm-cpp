// Links every available implementation (tests/headers/*.cpp) into one binary
// and includes the umbrella header, so the 26 libraries can live together.
#include <llm/llm.hpp>

#include <cstdio>
#include <cstring>

// The numeric macros must agree with the string (1.2.0 and 1.2.1 shipped with
// MINOR 1 / PATCH 0, so an #if on them gave the wrong answer).
#define LLM_CPP_STR2(x) #x
#define LLM_CPP_STR(x) LLM_CPP_STR2(x)
static const char kNumeric[] = LLM_CPP_STR(LLM_CPP_VERSION_MAJOR) "."
                               LLM_CPP_STR(LLM_CPP_VERSION_MINOR) "."
                               LLM_CPP_STR(LLM_CPP_VERSION_PATCH);

int main() {
    if (std::strcmp(kNumeric, LLM_CPP_VERSION) != 0) {
        std::printf("version macros disagree: %s vs %s\n", kNumeric, LLM_CPP_VERSION);
        return 1;
    }
    std::printf("llm-cpp %s (%d.%d.%d)\n", LLM_CPP_VERSION,
                LLM_CPP_VERSION_MAJOR, LLM_CPP_VERSION_MINOR, LLM_CPP_VERSION_PATCH);
    return std::strcmp(LLM_CPP_VERSION, LLM_CPP_EXPECTED_VERSION) == 0 ? 0 : 1;
}
