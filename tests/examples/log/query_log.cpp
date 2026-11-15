#define LLM_LOG_IMPLEMENTATION
#include "llm_log.hpp"
#include <iostream>
#include <cstdio>

int main() {
    llm::LogConfig cfg; cfg.filepath="query_test.jsonl";
    llm::Logger logger(cfg);
    std::vector<std::string> models={"gpt-4o-mini","claude-3-5-haiku-20241022"};
    for (int i=0;i<20;++i) {
        llm::LogEntry e; e.model=models[i%2]; e.prompt="Prompt "+std::to_string(i);
        e.response="Resp "+std::to_string(i); e.input_tokens=20+i; e.output_tokens=10+i;
        e.cost_usd=0.00001*(i+1); e.latency_ms=100.0+i*10; e.success=(i%7!=0);
        if(!e.success) e.error="simulated error";
        logger.log(e);
    }
    llm::Logger::QueryOptions opts; opts.model_filter="gpt-4o-mini"; opts.success_only=true; opts.limit=5;
    auto res=logger.query(opts);
    std::cout<<"gpt-4o-mini successes (limit 5): "<<res.size()<<"\n";
    auto s=logger.summarize();
    std::cout<<"total="<<s.total_calls<<" ok="<<s.successful_calls<<" fail="<<s.failed_calls
             <<" cost=$"<<s.total_cost_usd<<" tokens="<<s.total_tokens<<" avg_lat="<<s.avg_latency_ms<<"ms\n";
    for(auto& kv:s.calls_per_model) std::cout<<"  "<<kv.first<<": "<<kv.second<<" calls\n";
    std::remove("query_test.jsonl");
    return 0;
}