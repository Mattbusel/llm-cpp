#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cstdio>

int main() {
    llm::LogConfig cfg; cfg.filepath="scoped.jsonl"; cfg.also_stdout=true;
    llm::Logger logger(cfg);
    std::cout << "--- ScopedCall example ---\n\n";
    {
        llm::Logger::ScopedCall call(logger, "gpt-4o-mini", "What is 2+2?", {{"user","u123"}});
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        call.set_response("4"); call.set_tokens(10,5); call.set_cost(0.000008);
    }
    {
        llm::Logger::ScopedCall call(logger, "gpt-4o-mini", "Translate hello.");
        try { throw std::runtime_error("timeout"); } catch (const std::exception& ex) { call.set_error(ex.what()); }
    }
    std::cout << "\nCheck scoped.jsonl for output.\n";
    std::remove("scoped.jsonl");
    return 0;
}
