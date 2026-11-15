#define LLM_TRACE_IMPLEMENTATION
#include "llm_trace.hpp"
#include <cstdio>
#include <thread>
#include <chrono>

int main() {
    auto& tracer = llm::Tracer::global();
    tracer.clear();

    for (int i = 0; i < 3; ++i) {
        auto span = tracer.start("inference");
        span.set_model("gpt-4o-mini");
        span.set_tokens(200 + i * 50, 100 + i * 20);
        span.set_cost(0.00005 * (i + 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(5 + i * 3));
    }

    // Export OTLP-compatible JSON
    std::string json = tracer.export_json();
    std::printf("%s\n", json.c_str());
    return 0;
}
