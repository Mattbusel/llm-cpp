// Header hygiene checks, built with NOMINMAX defined on the command line
// (CMake: target_compile_definitions(... NOMINMAX)), which is how many Windows
// projects build. Up to llm-cpp 1.2.1, 15 headers did an unguarded
// #define NOMINMAX, a macro redefinition (MSVC C4005, GCC/Clang "redefined")
// that fails a /WX or -Werror build. They now define it only on Windows and
// only when it is not defined yet. With LLM_CPP_STRICT_WARNINGS this test is
// built with warnings as errors.
#define LLM_TRACE_IMPLEMENTATION
#include <llm/llm.hpp>

#include <cstdio>

static void two_spans_in_one_scope() {
    // Up to 1.2.1 LLM_TRACE pasted __LINE__ unexpanded, so a second use in the
    // same scope redeclared the same variable and did not compile.
    LLM_TRACE("first");
    LLM_TRACE("second");
}

int main() {
    two_spans_in_one_scope();
    const size_t n = llm::Tracer::global().completed_spans().size();
    std::printf("spans recorded: %zu (expected 2)\n", n);
    return n == 2 ? 0 : 1;
}
