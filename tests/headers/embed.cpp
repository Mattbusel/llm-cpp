// Compile test for llm_embed.hpp: this translation unit holds the
// implementation; tests/second_tu.cpp.in includes the same header again in a
// second translation unit with main(), so the test only links if the header
// is safe to include from several files.
#define LLM_EMBED_IMPLEMENTATION
#include <llm/llm_embed.hpp>
