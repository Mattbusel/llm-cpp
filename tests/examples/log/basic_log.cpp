#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"
#include <iostream>
#include <cstdio>

int main() {
    llm::LogConfig cfg;
    cfg.filepath    = "test_calls.jsonl";
    cfg.also_stdout = true;

    llm::Logger logger(cfg);

    { llm::LogEntry e; e.model="gpt-4o-mini"; e.prompt="Capital of France?"; e.response="Paris.";
      e.input_tokens=15; e.output_tokens=3; e.cost_usd=0.000012; e.latency_ms=320.5; e.success=true; logger.log(e); }
    { llm::LogEntry e; e.model="claude-3-5-haiku-20241022"; e.prompt="Explain recursion."; e.response="A function calling itself.";
      e.input_tokens=20; e.output_tokens=10; e.cost_usd=0.000025; e.latency_ms=450.0; e.success=true; logger.log(e); }
    { llm::LogEntry e; e.model="gpt-4o-mini"; e.prompt="Generate poem."; e.success=false;
      e.error="Rate limit exceeded"; e.latency_ms=100.0; logger.log(e); }

    std::cout << "\nWrote 3 entries to " << cfg.filepath << "\n";
    std::remove("test_calls.jsonl");
    return 0;
}
