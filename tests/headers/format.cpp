// Compile test for llm_format.hpp: this translation unit holds the
// implementation; tests/second_tu.cpp.in includes the same header again in a
// second translation unit with main(), so the test only links if the header
// is safe to include from several files.
#define LLM_FORMAT_IMPLEMENTATION
#include <llm/llm_format.hpp>
