#define LLM_POOL_IMPLEMENTATION
#include "llm_pool.hpp"
#include <iostream>
#include <mutex>

int main() {
    llm::PoolConfig cfg;
    cfg.max_concurrent      = 5;
    cfg.max_queue_size      = 50;
    cfg.requests_per_minute = 12;
    cfg.tokens_per_minute   = 10000;
    cfg.queue_timeout_ms    = 2000.0;

    llm::Pool pool(cfg);
    std::mutex print_mutex;

    std::cout << "Submitting 8 requests with rate limit 12/min...\n";
    for (int i = 0; i < 8; ++i) {
        llm::PoolRequest req;
        req.id = "r" + std::to_string(i);
        req.fn = [i, &print_mutex](){
            std::lock_guard<std::mutex> lk(print_mutex);
            std::cout << "  r" << i << " executed\n";
        };
        req.estimated_tokens = 100;
        pool.submit(std::move(req), [&print_mutex](llm::PoolResult r){
            if (!r.success) {
                std::lock_guard<std::mutex> lk(print_mutex);
                std::cout << "  " << r.request_id << " FAILED: " << r.error << "\n";
            }
        });
    }
    pool.drain();
    auto s = pool.stats();
    std::cout << "rate_limited=" << s.rate_limited
              << " completed=" << s.completed
              << " rejected=" << s.rejected << "\n";
    return 0;
}
