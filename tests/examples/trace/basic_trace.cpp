#define LLM_TRACE_IMPLEMENTATION
#include "llm_trace.hpp"
#include <cstdio>
#include <thread>
#include <chrono>

int main() {
    auto& tracer = llm::Tracer::global();

    {
        auto span = tracer.start("pipeline");
        span.set_model("gpt-4o-mini");
        span.set_tokens(512, 128);
        span.set_cost(0.0001);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto child = span.child("retrieval");
        child.set_attribute("index", "knowledge_base");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } // spans auto-end here

    std::printf("=== Text Export ===\n%s\n", tracer.export_text().c_str());
    return 0;
}
