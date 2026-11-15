#define LLM_POOL_IMPLEMENTATION
#include "llm_pool.hpp"
#include <iostream>
#include <mutex>

int main() {
    llm::PoolConfig cfg;
    cfg.max_concurrent      = 3;
    cfg.max_queue_size      = 100;
    cfg.requests_per_minute = 600;
    cfg.tokens_per_minute   = 200000;

    llm::Pool pool(cfg);
    std::mutex print_mutex;

    std::cout << "Submitting 10 requests with 3 concurrent slots...\n\n";
    for (int i = 0; i < 10; ++i) {
        llm::PoolRequest req;
        req.id       = "req-" + std::to_string(i);
        req.priority = llm::Priority::Normal;
        req.fn       = [i, &print_mutex]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            std::lock_guard<std::mutex> lk(print_mutex);
            std::cout << "  Executed req-" << i << "\n";
        };
        pool.submit(std::move(req), [&print_mutex](llm::PoolResult r) {
            std::lock_guard<std::mutex> lk(print_mutex);
            std::cout << "  Done: " << r.request_id
                      << " wait=" << (int)r.wait_ms
                      << "ms exec=" << (int)r.execute_ms << "ms\n";
        });
    }
    pool.drain();
    auto s = pool.stats();
    std::cout << "\nStats: completed=" << s.completed
              << " rejected=" << s.rejected
              << " avg_wait=" << (int)s.avg_wait_ms
              << "ms avg_exec=" << (int)s.avg_execute_ms << "ms\n";
    return 0;
}
