// Compile test for llm_guard.hpp: this translation unit holds the
// implementation; tests/second_tu.cpp.in includes the same header again in a
// second translation unit with main(), so the test only links if the header
// is safe to include from several files.
#define LLM_GUARD_IMPLEMENTATION
#include <llm/llm_guard.hpp>
