#define LLM_TRACE_IMPLEMENTATION
#include "llm_trace.hpp"
#include <cstdio>
#include <thread>
#include <chrono>

static void simulate_llm_call(llm::Tracer::ActiveSpan& parent, const char* name) {
    auto span = parent.child(name);
    span.set_model("gpt-4o-mini");
    span.set_tokens(100, 50);
    span.set_cost(0.00002);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
}

int main() {
    auto& tracer = llm::Tracer::global();

    {
        auto root = tracer.start("multi_step_pipeline");
        root.set_attribute("user_id", "u123");

        simulate_llm_call(root, "classify_intent");
        simulate_llm_call(root, "generate_response");
        simulate_llm_call(root, "validate_output");
    }

    std::printf("%s\n", tracer.export_text().c_str());

    // Also show JSON
    std::printf("JSON (first 200 chars):\n%.200s...\n", tracer.export_json().c_str());
    return 0;
}
