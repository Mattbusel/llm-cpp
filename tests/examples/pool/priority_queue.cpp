#define LLM_POOL_IMPLEMENTATION
#include "llm_pool.hpp"
#include <iostream>
#include <mutex>

int main() {
    llm::PoolConfig cfg;
    cfg.max_concurrent      = 1;
    cfg.max_queue_size      = 100;
    cfg.requests_per_minute = 600;
    cfg.tokens_per_minute   = 200000;

    llm::Pool pool(cfg);
    std::mutex print_mutex;

    // Block the single worker while we enqueue the rest
    pool.submit({"blocker", [](){ std::this_thread::sleep_for(std::chrono::milliseconds(200)); },
                 llm::Priority::Normal, 0}, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    for (int i = 0; i < 3; ++i)
        pool.submit({"low-" + std::to_string(i),
                     [i, &print_mutex](){
                         std::lock_guard<std::mutex> lk(print_mutex);
                         std::cout << "  LOW-" << i << " ran\n";
                     },
                     llm::Priority::Low, 10}, nullptr);

    for (int i = 0; i < 3; ++i)
        pool.submit({"high-" + std::to_string(i),
                     [i, &print_mutex](){
                         std::lock_guard<std::mutex> lk(print_mutex);
                         std::cout << "  HIGH-" << i << " ran\n";
                     },
                     llm::Priority::High, 10}, nullptr);

    pool.submit({"critical-0",
                 [&print_mutex](){
                     std::lock_guard<std::mutex> lk(print_mutex);
                     std::cout << "  CRITICAL-0 ran\n";
                 },
                 llm::Priority::Critical, 10}, nullptr);

    std::cout << "Priority order (CRITICAL > HIGH > LOW expected):\n";
    pool.drain();
    return 0;
}
