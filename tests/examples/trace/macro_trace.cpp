#define LLM_TRACE_IMPLEMENTATION
#include "llm_trace.hpp"
#include <cstdio>
#include <thread>
#include <chrono>

void step_one() {
    LLM_TRACE("step_one");
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
}

void step_two() {
    LLM_TRACE("step_two");
    std::this_thread::sleep_for(std::chrono::milliseconds(12));
}

int main() {
    {
        LLM_TRACE("main_pipeline");
        step_one();
        step_two();
    }

    std::printf("%s\n", llm::Tracer::global().export_text().c_str());
    return 0;
}
