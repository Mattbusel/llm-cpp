// Compile test for llm_ab.hpp: this translation unit holds the
// implementation; tests/second_tu.cpp.in includes the same header again in a
// second translation unit with main(), so the test only links if the header
// is safe to include from several files.
#define LLM_AB_IMPLEMENTATION
#include <llm/llm_ab.hpp>
